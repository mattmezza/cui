# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM opt-in boxed/bare tooltip appearance in both consumers."""
import argparse
import ast
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--chromium');parser.add_argument('--output',type=Path,default=ROOT/'build/tooltip-appearance-probe')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    tree=ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    sources=[s for s in sources if s not in ['web/gallery.c','examples/composed_apps/app.c']]+['tests/web/chart_tooltip_appearance_probe.c','web/modules/cui_shadow_bridge.c']
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['probe_init','probe_frame','probe_calls','probe_focus','probe_bounds','probe_count','probe_command','probe_text','probe_length','probe_pixels','probe_bitmap','probe_shadow','probe_shadow_coverage','cui_canvas_shadow_values','cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width','cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound','cui_canvas_shadow_pixels'];wasm=out/'probe.wasm'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s) for s in sources],'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}' for s in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}));page=browser.new_page()
        page.set_content('<canvas width="500" height="400"></canvas>')
        results=page.evaluate('''async input=>{
          const [bytes,moduleSource]=input;const mod=await import(URL.createObjectURL(new Blob([moduleSource],{type:'text/javascript'})));
          const m=await WebAssembly.compile(new Uint8Array(bytes)),imports={};
          for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();if(e.probe_init())throw Error('init');
          const ctx=document.querySelector('canvas').getContext('2d');
          const rasterize=mod.cuiShadowWasmRasterizer(e),renderer=mod.createCuiShadowRenderer(rasterize);
          const frame=(source,legacy,left=0,width=240)=>{const before=e.probe_calls();
            if(e.probe_frame(source,legacy,left,width))throw Error('frame');
            const decoded=Array.from({length:e.probe_count()},(_,i)=>{const c=Array.from({length:12},(_,j)=>e.probe_command(i,j));return c[0]===7?{type:'shadow',values:Array.from({length:16},(_,j)=>e.probe_shadow(i,j))}:c[0]===3?{type:'clipPush',bounds:c.slice(1,5)}:c[0]===4?{type:'clipPop'}:{type:'other'};});
            const prepared=renderer.prepare(decoded,ctx,500,400);
            ctx.fillStyle='white';ctx.fillRect(0,0,500,400);let depth=0;let clips=[];const texts=[];let boxHeight=0;let surfaces=[];let borders=0;let rings=[];let shadows=[];let shadowIndices=[];
            for(let i=0;i<e.probe_count();i++){
              const c=Array.from({length:12},(_,j)=>e.probe_command(i,j));
              if(c[0]===7){const v=decoded[i].values;shadows.push(v);shadowIndices.push(i);while(depth){ctx.restore();depth--;}prepared.draw(i,ctx,{unclipped:true});for(const clip of clips){ctx.save();depth++;ctx.beginPath();ctx.rect(...clip);ctx.clip();}if(v[10]===0)rings.push([7,v[0]-v[11],v[1]-v[11],v[2]+2*v[11],v[3]+2*v[11],...v.slice(12),0,v[4],0]);}
              else if(c[0]===3){clips.push(c.slice(1,5));ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
              else if(c[0]===4){clips.pop();ctx.restore();depth--;}
              else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.beginPath();ctx.roundRect(...c.slice(1,5),c[10]);ctx.fill();if(c[5]===240&&c[6]===230)surfaces.push(c.slice(1,5));if(c[5]===220&&c[6]===20)rings.push(c);}
              else if(c[0]===1){if(c[3]>=150)borders++;ctx.strokeStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.lineWidth=c[11];ctx.beginPath();ctx.roundRect(c[1]+c[11]/2,c[2]+c[11]/2,c[3]-c[11],c[4]-c[11],Math.max(0,c[10]-c[11]/2));ctx.stroke();if(c[3]>=150)boxHeight=c[4];}
              else if(c[0]===2){const text=new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_length(i)));ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.font=`${c[9]}px sans-serif`;ctx.fillText(text,c[1],c[2]+c[9]);texts.push({text,rect:c.slice(1,5)});}
              else if(c[0]===5){const bw=e.probe_bitmap(i,0),bh=e.probe_bitmap(i,1),stride=e.probe_bitmap(i,2),ptr=e.probe_pixels(i);const temp=document.createElement('canvas');temp.width=bw;temp.height=bh;const pixels=new Uint8ClampedArray(bw*bh*4);for(let y=0;y<bh;y++)pixels.set(new Uint8Array(e.memory.buffer,ptr+y*stride,bw*4),y*bw*4);temp.getContext('2d').putImageData(new ImageData(pixels,bw,bh),0,0);ctx.drawImage(temp,...c.slice(1,5));}
              else throw Error('command');
            }
            if(depth)throw Error('clip');
            for(const t of texts){const r=t.rect.map(Math.ceil),data=ctx.getImageData(r[0],r[1],Math.max(1,r[2]),Math.max(1,r[3])).data;t.ink=Array.from({length:data.length/4},(_,i)=>data[i*4]<160||data[i*4+1]<160||data[i*4+2]<160).filter(Boolean).length;}
            const sample=(x,y)=>Array.from(ctx.getImageData(x,y,1,1).data);
            const outside=surfaces.length?sample(surfaces[0][0]+40,surfaces[0][1]-1):null;
            const inside=surfaces.length?sample(surfaces[0][0]+40,surfaces[0][1]+1):null;
            const shadowExpected=(x,y,surfaceAlpha=0)=>{let color=[255,255,255];for(let n=0;n<shadows.length;n++){const v=shadows[n],a=Math.round(e.probe_shadow_coverage(shadowIndices[n],x+.5,y+.5)*255)/255*v[15]/255;color=color.map((c,k)=>v[12+k]*a+c*(1-a));}if(surfaceAlpha)color=color.map((c,k)=>[240,230,210][k]*surfaceAlpha/255+c*(1-surfaceAlpha/255));return color.map(Math.round).concat(255);};
            const surfaceAlpha=surfaces.length?Array.from({length:e.probe_count()},(_,i)=>Array.from({length:12},(_,j)=>e.probe_command(i,j))).find(c=>c[0]===0&&c[5]===240&&c[6]===230)[8]:0;
            const outsideExpected=surfaces.length?shadowExpected(surfaces[0][0]+40,surfaces[0][1]-1):null;
            const insideExpected=surfaces.length?shadowExpected(surfaces[0][0]+40,surfaces[0][1]+1,surfaceAlpha):null;
            let corner=null,oldCorner=null;
            if(surfaces.length){const r=rings[0],box=surfaces[0];corner=sample(box[0]+1,box[1]+2);
              const ref=document.createElement('canvas');ref.width=500;ref.height=400;const rc=ref.getContext('2d');rc.fillStyle='white';rc.fillRect(0,0,500,400);
              rc.fillStyle=`rgba(${r[5]},${r[6]},${r[7]},${r[8]/255})`;rc.beginPath();rc.roundRect(...r.slice(1,5),r[10]+1);rc.fill();
              rc.fillStyle='rgb(240,230,210)';rc.beginPath();rc.roundRect(...box,10);rc.fill();oldCorner=Array.from(rc.getImageData(box[0]+1,box[1]+2,1,1).data);}

            return {texts,surfaces,borders,rings,shadows,outside,inside,outsideExpected,insideExpected,corner,oldCorner,focus:e.probe_focus(),boxHeight,calls:e.probe_calls()-before,bounds:e.probe_bounds(),ink:Array.from(ctx.getImageData(0,0,500,400).data).filter((v,i)=>i%4!==3&&v<200).length};
          };
          return [frame(0,0),frame(1,0),frame(2,0),frame(3,0),frame(0,1),frame(1,1),frame(2,1),frame(3,1),frame(1,0,10,260),frame(4,0,0,240),frame(4,1,0,240),frame(8,0,0,240),frame(8,1,0,240),frame(24,0,0,240),frame(24,1,0,240)];
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()])
        get=lambda r,text:next(t['rect'] for t in r['texts'] if t['text']==text)
        for i,result in enumerate(results):
            automatic=i%4>=2 if i<8 else False
            bare=i%2==1 if i<8 else i==8
            assert result['calls']==(0 if automatic else 1),result
            assert result['borders']==0,result
            assert len(result['rings'])==(0 if bare else 1),result
            assert len(result['shadows'])==(0 if bare else 3),result
            if not bare:
                assert [v[8:12] for v in result['shadows']]==[[0,0,0,1],[0,4,3,-1],[0,2,2,-2]],result
            assert bool(result['surfaces']) != bare,result
            for value in (['Default','Chart','20'] if automatic else ['Totals','Series','Total','20','1000']):
                assert any(t['text']==value and t['ink']>0 for t in result['texts']),result
            label=get(result,'Chart' if automatic else 'Series')
            if bare:
                assert label[0]==(24 if i==8 else 14),result
                value=get(result,'20' if automatic else '1000')
                assert value[0]+value[2]==(270 if i==8 else 240),result
            else:
                assert result['surfaces'][0]==[28,28,150,60 if automatic else 84],result
                assert label[0]==50,result
                ring=result['rings'][0];assert ring[1:5]==[27,27,152,86 if not automatic else 62],result
                assert ring[8]==(13 if i in [9,10,13,14] else 26) and ring[10]==10 and ring[11]==0,result
                assert all(abs(a-b)<=1 for a,b in zip(result['outside'],result['outsideExpected'])),result
                assert all(abs(a-b)<=1 for a,b in zip(result['inside'],result['insideExpected'])),result
                sa=64 if i>=13 else 128 if i>=11 else 127 if i>=9 else 255
                if i<9: assert result['corner'][1]<result['oldCorner'][1],result
                if i>=9:
                    without_ring=round(230*sa/255+255*(1-sa/255))
                    assert result['inside'][1]<=without_ring-5,result
                if i>=13: assert abs(result['focus']-.5)<.001,result
            if not automatic:
                assert get(result,'Total')[1]-label[1]==24,result
        assert results[8]['bounds']==10
        page.screenshot(path=str(out/'tooltip-appearance.png'));browser.close()
    print('PASS copied style, source metrics/roles, boxed/bare automatic and typed content, both consumers, stationary owner move/resize and source filled shadow radius/translucent/progress compositing WASM pixels')
if __name__=='__main__':main()
