# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Standalone WASM theme seam driven by actual browser preference events."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
HTML='''<!doctype html><canvas width="200" height="100"></canvas><script>
(async()=>{
 const module=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer());
 const imports={};for(const i of WebAssembly.Module.imports(module)){(imports[i.module]??={})[i.name]=i.name==='proc_exit'?()=>{throw Error('WASI exit')}:()=>0;}
 const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();if(e.theme_probe_init())throw Error('init');
 const dark=matchMedia('(prefers-color-scheme: dark)'),reduce=matchMedia('(prefers-reduced-motion: reduce)'),contrast=matchMedia('(forced-colors: active)');
 let revision=0;window.themeProbe={e,events:0};
 const update=()=>{if(e.theme_probe_update(Number(dark.matches),Number(reduce.matches),Number(contrast.matches),++revision))throw Error('update');
  const data=Array.from({length:14},(_,i)=>e.theme_probe_stat(i));themeProbe.data=data;themeProbe.events++;
  const c=document.querySelector('canvas'),ctx=c.getContext('2d');ctx.fillStyle=`rgb(${data[4]},${data[5]},${data[6]})`;ctx.fillRect(0,0,200,100);
  ctx.fillStyle='white';ctx.font=`${data[7]}px sans-serif`;ctx.fillText('Theme',10,50);
 };
 for(const media of [dark,reduce,contrast])media.addEventListener('change',update);update();
})().catch(error=>{window.themeError=String(error);throw error;});
</script>'''
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'build/theme-probe')
    parser.add_argument('--zig',default='zig')
    parser.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    assert subprocess.check_output([args.zig,'version'],text=True).strip()=='0.16.0'
    subprocess.run([args.zig,'cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),
        *[str(ROOT/p) for p in ['tests/web/theme_registry_probe.c','src/theme/cui_theme.c','src/theme/cui_theme_motion.c','src/theme/cui_theme_registry.c']],
        '-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}' for s in ['theme_probe_init','theme_probe_update','theme_probe_stat']],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
            page.emulate_media(color_scheme='light',reduced_motion='no-preference',forced_colors='none')
            page.goto(f'http://127.0.0.1:{server.server_port}')
            page.wait_for_function('window.themeProbe?.data?.length===14')
            data=page.evaluate('themeProbe.data');assert data[0]==40 and data[1]==0 and data[4:8]==[18,69,120,23]
            assert page.evaluate("Array.from(document.querySelector('canvas').getContext('2d').getImageData(0,0,1,1).data)")==[18,69,120,255]
            page.emulate_media(color_scheme='dark')
            page.wait_for_function('themeProbe.data[1]===1')
            assert page.evaluate('themeProbe.data.slice(4,8)')==[34,51,17,20]
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('themeProbe.data[2]===1&&themeProbe.data[8]===0&&themeProbe.data[9]===0')
            page.emulate_media(forced_colors='active')
            page.wait_for_function('themeProbe.data[3]===1')
            assert page.evaluate('themeProbe.data.slice(4,7)')==[0,0,0]
            assert page.evaluate("Array.from(document.querySelector('canvas').getContext('2d').getImageData(0,0,1,1).data)")==[0,0,0,255]
            assert page.evaluate('themeProbe.events')>=4
            # Repeated preferences do not replay a visual/metrics/motion change.
            assert page.evaluate('''()=>{const e=themeProbe.e,r=e.theme_probe_stat(13);return e.theme_probe_update(1,1,1,r)===0&&[10,11,12].every(i=>e.theme_probe_stat(i)===0)}''')
            assert not errors,errors;browser.close()
    finally:server.shutdown()
    print('WASM source semantic import + matchMedia host appearance/reduced/contrast seam PASS')
if __name__=='__main__':main()
