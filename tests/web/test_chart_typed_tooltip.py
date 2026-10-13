# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM copied typed-tooltip rows in legacy and current-frame paint."""
import argparse
import ast
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--chromium');parser.add_argument('--output',type=Path,default=ROOT/'build/typed-tooltip-probe')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    tree=ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    sources=[s for s in sources if s not in ['web/gallery.c','examples/composed_apps/app.c']]+['tests/web/chart_typed_tooltip_probe.c']
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['probe_init','probe_frame','probe_calls','probe_bounds','probe_count','probe_command','probe_text','probe_length','probe_pixels','probe_bitmap'];wasm=out/'probe.wasm'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s) for s in sources],'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}' for s in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}));page=browser.new_page()
        page.set_content('<canvas width="240" height="220"></canvas>')
        results=page.evaluate('''async bytes=>{
          const m=await WebAssembly.compile(new Uint8Array(bytes)),imports={};
          for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();if(e.probe_init())throw Error('init');
          const ctx=document.querySelector('canvas').getContext('2d');
          const frame=(source,legacy,left=0,width=128)=>{const before=e.probe_calls();
            if(e.probe_frame(source,legacy,left,width))throw Error('frame');
            ctx.fillStyle='white';ctx.fillRect(0,0,240,220);let depth=0;const texts=[];let boxHeight=0;
            for(let i=0;i<e.probe_count();i++){
              const c=Array.from({length:10},(_,j)=>e.probe_command(i,j));
              if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
              else if(c[0]===4){ctx.restore();depth--;}
              else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
              else if(c[0]===1){ctx.strokeStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.strokeRect(...c.slice(1,5));if(c[3]>=150)boxHeight=c[4];}
              else if(c[0]===2){const text=new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_length(i)));ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.font=`${c[9]}px sans-serif`;ctx.fillText(text,c[1],c[2]+c[9]);texts.push({text,rect:c.slice(1,5)});}
              else if(c[0]===5){const bw=e.probe_bitmap(i,0),bh=e.probe_bitmap(i,1),stride=e.probe_bitmap(i,2),ptr=e.probe_pixels(i);const temp=document.createElement('canvas');temp.width=bw;temp.height=bh;const pixels=new Uint8ClampedArray(bw*bh*4);for(let y=0;y<bh;y++)pixels.set(new Uint8Array(e.memory.buffer,ptr+y*stride,bw*4),y*bw*4);temp.getContext('2d').putImageData(new ImageData(pixels,bw,bh),0,0);ctx.drawImage(temp,...c.slice(1,5));}
              else throw Error('command');
            }
            if(depth)throw Error('clip');
            for(const t of texts){const r=t.rect.map(Math.ceil),data=ctx.getImageData(r[0],r[1],Math.max(1,r[2]),Math.max(1,r[3])).data;t.ink=Array.from({length:data.length/4},(_,i)=>data[i*4]<160||data[i*4+1]<160||data[i*4+2]<160).filter(Boolean).length;}
            return {texts,boxHeight,calls:e.probe_calls()-before,bounds:e.probe_bounds(),ink:Array.from(ctx.getImageData(0,0,240,220).data).filter((v,i)=>i%4!==3&&v<200).length};
          };
          return [frame(0,0),frame(1,0),frame(2,0),frame(0,1),frame(1,1),frame(3,0),frame(3,1),frame(1,0,10,200)];
        }''',list(wasm.read_bytes()))
        for result in results:
            assert result['calls']==1,result
        for result in results[:5]+results[7:]:
            for value in ['Totals','Series','Total','20','1000']:
                assert any(t['text']==value and t['ink']>0 for t in result['texts']),result
        get=lambda r,text:next(t['rect'] for t in r['texts'] if t['text']==text)
        for plain,mixed in [(results[0],results[1]),(results[3],results[4])]:
            assert get(mixed,'Series')[0]==get(plain,'Series')[0]+14
            assert get(mixed,'Series')[0]==get(mixed,'Total')[0]
            assert get(plain,'Series')[0]==get(plain,'Total')[0]
            v1=get(mixed,'20');v2=get(mixed,'1000');assert abs(v1[0]+v1[2]-v2[0]-v2[2])<.01
        assert get(results[2],'Series')[0]==get(results[1],'Series')[0]
        assert not results[5]['texts'] and results[5]['ink']==0
        assert not results[6]['texts'] and results[6]['ink']==0
        assert results[7]['bounds']==10
        page.screenshot(path=str(out/'typed-tooltip.png'));browser.close()
    print('PASS copied plain/mixed/transparent rows, alignment, legacy/current rollback and snapshot WASM canvas pixels')
if __name__=='__main__':main()
