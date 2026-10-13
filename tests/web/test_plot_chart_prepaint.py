# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Chart/Plot WASM current-frame prepaint proof using real Chromium canvas pixels."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><style>body{margin:0}canvas{display:block;margin:10px}</style>
<canvas height="160"></canvas><label><input id="overlay" type="checkbox">Passive overlay</label>
<label><input id="modal" type="checkbox">Modal</label><script>
(async()=>{
 const module=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer());
 const imports={};for(const i of WebAssembly.Module.imports(module)){(imports[i.module]??={})[i.name]=i.name==='proc_exit'?()=>{throw Error('WASI exit')}:()=>0;}
 const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();if(e.probe_init())throw Error('init');
 const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');window.probe={e,frames:0,signals:[]};
 const frame=()=>{
  canvas.width=Math.min(240,innerWidth-20);if(e.probe_frame(canvas.width,160))throw Error('frame');
  ctx.clearRect(0,0,canvas.width,160);let depth=0;
  for(let i=0;i<e.probe_count();i++){
   const c=Array.from({length:10},(_,j)=>e.probe_command(i,j));
   if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
   else if(c[0]===4){if(!depth)throw Error('clip underflow');ctx.restore();depth--;}
   else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
   else if(c[0]===1){ctx.strokeStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.strokeRect(...c.slice(1,5));}
   else if(c[0]===2){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.font=`${c[9]}px sans-serif`;ctx.fillText(new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_length(i))),c[1],c[2]+c[9]);}
   else if(c[0]===5){const bw=e.probe_bitmap(i,0),bh=e.probe_bitmap(i,1),stride=e.probe_bitmap(i,2),p=e.probe_pixels(i);const temp=document.createElement('canvas');temp.width=bw;temp.height=bh;const tc=temp.getContext('2d'),pixels=new Uint8ClampedArray(bw*bh*4);for(let row=0;row<bh;row++)pixels.set(new Uint8Array(e.memory.buffer,p+row*stride,bw*4),row*bw*4);tc.putImageData(new ImageData(pixels,bw,bh),0,0);ctx.drawImage(temp,...c.slice(1,5));}
   else throw Error('unexpected command '+c[0]);
  }
  if(depth)throw Error('unbalanced clip');probe.frames++;probe.data=Array.from({length:14},(_,i)=>e.probe_stat(i));
 };
 canvas.addEventListener('pointermove',ev=>{const r=canvas.getBoundingClientRect();if(!e.probe_event(0,ev.clientX-r.left,ev.clientY-r.top,0))throw Error('queue');probe.signals.push('pointer');frame();});
 canvas.addEventListener('wheel',ev=>{ev.preventDefault();const r=canvas.getBoundingClientRect();if(!e.probe_event(1,ev.clientX-r.left,ev.clientY-r.top,ev.deltaY))throw Error('queue');probe.signals.push('wheel');frame();},{passive:false});
 window.addEventListener('resize',()=>{probe.signals.push('resize');frame();});
 for(const id of ['overlay','modal'])document.getElementById(id).addEventListener('change',()=>{e.probe_options(Number(overlay.checked),Number(modal.checked));probe.signals.push(id);frame();});
 frame();
})().catch(error=>{window.probeError=String(error);throw error;});
</script>'''
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'build/plot-chart-prepaint-probe')
    parser.add_argument('--chromium')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    tree=ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources=next(ast.literal_eval(n.value)for n in tree.body if isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='SOURCES'for t in n.targets))
    sources=[s for s in sources if s not in ['web/gallery.c','examples/composed_apps/app.c']]+['tests/web/plot_chart_prepaint_probe.c']
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['probe_init','probe_event','probe_options','probe_frame','probe_stat','probe_count','probe_command','probe_text','probe_length','probe_pixels','probe_bitmap']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s)for s in sources],'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}'for s in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright()as p:
            browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}))
            page=browser.new_page(viewport={'width':300,'height':300});errors=[]
            page.on('pageerror',lambda error:errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe?.frames===1')
            def data():return page.evaluate('probe.data')
            def pixel(x,y):return page.evaluate('([x,y])=>Array.from(document.querySelector("canvas").getContext("2d").getImageData(x,y,1,1).data)',[x,y])
            def tooltip_pixels():return page.evaluate('''()=>{const p=document.querySelector('canvas').getContext('2d').getImageData(140,40,70,60).data;let n=0;for(let i=0;i<p.length;i+=4)if(p[i]<160&&p[i+1]<160&&p[i+2]<160&&p[i+3]>0)n++;return n;}''')
            assert data()[10]==-1 and tooltip_pixels()==0,data()
            # Real pointer then real wheel; wheel preserves the pointer while Chart origin changes.
            page.mouse.move(50,50);page.wait_for_function('probe.data[10]===0')
            assert tooltip_pixels()>0  # Actual default tooltip text beyond ancestor clip.
            before=data();page.mouse.wheel(0,15);page.wait_for_function('probe.data[8]===1')
            after=data();assert after[12]==before[12]-15 and after[13]==before[13]+15,after
            page.locator('#overlay').check();page.wait_for_function('probe.signals.includes("overlay")');assert data()[10]==-1,data()
            assert pixel(30,30)==[200,0,0,255] and tooltip_pixels()==0
            page.locator('#overlay').uncheck();page.wait_for_function('probe.data[10]===0')
            page.locator('#modal').check();page.wait_for_function('probe.signals.includes("modal")');assert data()[10]==-1,data()
            page.locator('#modal').uncheck();page.wait_for_function('probe.data[10]===0')
            # Plot current-frame callback and deferred paint escape its clipped viewport.
            page.mouse.move(40,140);page.wait_for_function('probe.data[3]===1')
            assert pixel(30,130)==[0,180,0,255] and pixel(160,130)==[240,200,0,255]
            # Resize with stationary pointer clips Chart interaction using current bounds.
            page.mouse.move(120,50);page.wait_for_function('probe.data[10]>=0')
            page.set_viewport_size({'width':180,'height':300});page.wait_for_function('probe.signals.includes("resize")')
            assert data()[10]==-1,data()
            assert not errors,errors;browser.close()
    finally:server.shutdown()
    print('Chart Plot current-frame WASM pixels passed')
if __name__=='__main__':main()
