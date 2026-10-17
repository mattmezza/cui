# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Icon numeric styling and shared-state raster lifetime pixels."""
import argparse,ast,subprocess,threading
from functools import partial
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
HTML='''<canvas id="view" style="width:160px;height:100px"></canvas><script type="module">
const wasi={proc_exit:c=>{throw Error('WASI exit '+c)},fd_close:()=>0,fd_seek:()=>0,fd_write:()=>0,
 environ_sizes_get:()=>0,environ_get:()=>0,clock_time_get:()=>0,random_get:()=>0};
const {instance}=await WebAssembly.instantiateStreaming(fetch('probe.wasm'),{wasi_snapshot_preview1:wasi});
const e=instance.exports;e._initialize?.();if(!e.icon_style_init())throw Error('init');
window.probe={e,draw(mode){if(!e.icon_style_frame(mode))throw Error('frame');
 const canvas=document.querySelector('canvas'),scale=devicePixelRatio;canvas.width=160*scale;canvas.height=100*scale;
 const off=new OffscreenCanvas(160,100),ctx=off.getContext('2d');
 ctx.putImageData(new ImageData(new Uint8ClampedArray(e.memory.buffer,e.icon_style_pixels(),160*100*4).slice(),160,100),0,0);
 canvas.getContext('2d').drawImage(off,0,0,canvas.width,canvas.height);
 return [e.icon_style_stat(0),e.icon_style_stat(1),e.icon_style_stat(2)];}};
probe.draw(0);
</script>'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'build/icon-style-probe');p.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'));a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
 tree=ast.parse((ROOT/'scripts/build_web.py').read_text());sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='SOURCES' for t in n.targets))
 sources=list(dict.fromkeys([str(ROOT/s) for s in sources if s.startswith('src/')]+[str(ROOT/'backends/software/cui_software.c')]))
 exports=['icon_style_init','icon_style_frame','icon_style_pixels','icon_style_stat']
 subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/icon_style_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}' for s in exports],'-o',str(out/'probe.wasm')],check=True)
 (out/'index.html').write_text(HTML)
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*_args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright() as pw:
   browser=pw.chromium.launch(headless=True,executable_path=a.chromium)
   for width in [1280,390]:
    for dpr in [1,2]:
     page=browser.new_page(viewport={'width':width,'height':600},device_scale_factor=dpr);errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe')
     assert page.evaluate('probe.draw(0)')==[16,16,1]
     normal=page.locator('canvas').screenshot();assert page.evaluate('probe.draw(1)')==[48,48,1];assert page.locator('canvas').screenshot()!=normal
     assert page.evaluate('probe.draw(2)')==[8,16,1]
     assert page.evaluate('probe.draw(3)')==[16,16,3]
     colors=page.evaluate('''()=>{const c=document.querySelector('canvas'),p=c.getContext('2d').getImageData(0,0,c.width,c.height).data;let rgb=[0,0,0];for(let i=0;i<p.length;i+=4)for(let k=0;k<3;k++)rgb[k]+=p[i+k]>200;return rgb;}''');assert all(colors),colors
     page.evaluate('probe.draw(4)')
     assert page.evaluate('''()=>{const c=document.querySelector('canvas'),s=devicePixelRatio,p=c.getContext('2d').getImageData(9*s,0,c.width-9*s,c.height).data;return !p.some((v,i)=>i%4!==3&&v);}''')
     assert page.evaluate('''()=>{const c=document.querySelector('canvas'),p=c.getContext('2d').getImageData(0,0,c.width,c.height).data;return p.some((v,i)=>i%4===0&&v>90&&v<150);}''')
     assert not errors,errors;page.close()
   browser.close()
  print('PASS real WASM Icon styling/shared-state pixels/clip/semantics: desktop390 DPR1/2')
 finally:server.shutdown();server.server_close()
if __name__=='__main__':main()
