# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM current-frame Radar tooltip title/row canvas proof."""
import argparse
import ast
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--chromium');parser.add_argument('--output',type=Path,default=ROOT/'build/radar-title-probe')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    tree=ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
    sources=[s for s in sources if s not in ['web/gallery.c','examples/composed_apps/app.c']]+['tests/web/chart_radar_title_probe.c']
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['probe_init','probe_frame','probe_hover','probe_count','probe_command','probe_text','probe_length','probe_pixels','probe_bitmap'];wasm=out/'probe.wasm'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s) for s in sources],'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}' for s in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}));page=browser.new_page()
        page.set_content('<canvas width="240" height="220"></canvas>')
        results=page.evaluate('''async bytes=>{
          const m=await WebAssembly.compile(new Uint8Array(bytes)),imports={};
          for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();if(e.probe_init())throw Error('init');
          const ctx=document.querySelector('canvas').getContext('2d');
          const frame=(source,override)=>{
            if(e.probe_frame(source,override))throw Error('frame');
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
            for(const t of texts){const r=t.rect.map(Math.ceil),data=ctx.getImageData(r[0],r[1],Math.max(1,r[2]),Math.max(1,r[3])).data;t.ink=Array.from({length:data.length/4},(_,i)=>data[i*4]<160&&data[i*4+1]<160&&data[i*4+2]<160).filter(Boolean).length;}
            return {texts,boxHeight,hover:e.probe_hover()};
          };
          return [frame(0,0),frame(1,0),frame(1,1)];
        }''',list(wasm.read_bytes()))
        for result in results:
            assert result['hover']==0,result
            assert any(t['text']=='20' and t['ink']>0 for t in result['texts']),result
        assert any(t['text']=='Top' and t['ink']>0 for t in results[0]['texts'])
        assert not any(t['text']=='Top' for t in results[1]['texts'])
        assert any(t['text']=='Override' and t['ink']>0 for t in results[2]['texts'])
        assert results[1]['boxHeight']<results[0]['boxHeight'],results
        page.screenshot(path=str(out/'radar-title.png'));browser.close()
    print('PASS Radar legacy/source/explicit titles and retained row WASM canvas pixels')
if __name__=='__main__':main()
