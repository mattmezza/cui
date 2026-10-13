# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Core-only WASM current-frame prepaint proof using real Chromium canvas pixels."""
import argparse
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
   const c=Array.from({length:9},(_,j)=>e.probe_command(i,j));
   if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
   else if(c[0]===4){if(!depth)throw Error('clip underflow');ctx.restore();depth--;}
   else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
   else throw Error('unexpected command '+c[0]);
  }
  if(depth)throw Error('unbalanced clip');probe.frames++;probe.data=Array.from({length:10},(_,i)=>e.probe_stat(i));
 };
 canvas.addEventListener('pointermove',ev=>{const r=canvas.getBoundingClientRect();if(!e.probe_event(0,ev.clientX-r.left,ev.clientY-r.top,0))throw Error('queue');probe.signals.push('pointer');frame();});
 canvas.addEventListener('wheel',ev=>{ev.preventDefault();const r=canvas.getBoundingClientRect();if(!e.probe_event(1,ev.clientX-r.left,ev.clientY-r.top,ev.deltaY))throw Error('queue');probe.signals.push('wheel');frame();},{passive:false});
 window.addEventListener('resize',()=>{probe.signals.push('resize');frame();});
 for(const id of ['overlay','modal'])document.getElementById(id).addEventListener('change',()=>{e.probe_options(Number(overlay.checked),Number(modal.checked));probe.signals.push(id);frame();});
 frame();
})().catch(error=>{window.probeError=String(error);throw error;});
</script>'''
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/core-prepaint-probe')
    parser.add_argument('--zig', default='zig')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output([args.zig, 'version'], text=True).strip() == '0.16.0'
    exports = ['probe_init', 'probe_event', 'probe_options', 'probe_frame', 'probe_stat', 'probe_count', 'probe_command']
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O2',
                    '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(ROOT/'include'), '-I', str(ROOT/'third_party/clay'),
                    str(ROOT/'tests/web/core_prepaint_probe.c'), str(ROOT/'src/core/cui.c'), '-lm',
                    '-Wl,-z,stack-size=8388608', *[f'-Wl,--export={s}' for s in exports], '-o', str(out/'probe.wasm')], check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(out)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=args.chromium)
            page = browser.new_page(viewport={'width': 300, 'height': 300})
            errors = []; page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}')
            page.wait_for_function('window.probe?.frames===1')
            def data(): return page.evaluate('probe.data')
            def pixel(x, y): return page.evaluate('([x,y])=>Array.from(document.querySelector("canvas").getContext("2d").getImageData(x,y,1,1).data)', [x, y])
            assert data()[2:5] == [0, 0, 0], data()
            assert pixel(30, 30) == [0, 0, 180, 255]
            assert pixel(5, 30) == [255, 255, 255, 255]  # Local paint cannot escape ancestor clip.
            assert pixel(150, 25) == [255, 255, 255, 255]
            page.mouse.move(40, 40)
            page.wait_for_function('probe.data[3]===1')
            assert data()[2:5] == [1, 1, 20]
            assert pixel(30, 30) == [0, 180, 0, 255]
            assert pixel(150, 25) == [240, 200, 0, 255]  # Deferred stream escapes local clip.
            page.locator('#overlay').focus(); page.keyboard.press('Space')
            page.wait_for_function('probe.data[4]===30')
            assert data()[3] == 0 and pixel(30, 30) == [200, 0, 0, 255]
            assert pixel(150, 25) == [255, 255, 255, 255]
            page.locator('#overlay').focus(); page.keyboard.press('Space')
            page.wait_for_function('probe.data[3]===1')
            page.locator('#modal').focus(); page.keyboard.press('Space')
            page.wait_for_function('probe.data[7]===0')
            assert data()[3] == 0 and pixel(30, 30) == [0, 0, 180, 255]
            assert pixel(190, 110) == [180, 0, 180, 255]
            assert pixel(150, 25) == [255, 255, 255, 255]
            page.locator('#modal').focus(); page.keyboard.press('Space')
            page.wait_for_function('probe.data[3]===1')
            page.mouse.move(85, 40)
            page.wait_for_function('probe.data[3]===1')
            pointer_signals = page.evaluate('probe.signals.filter(s=>s==="pointer").length')
            page.set_viewport_size({'width': 100, 'height': 300})
            page.wait_for_function('probe.data[6]===40&&probe.data[3]===0')
            assert page.evaluate('probe.signals.filter(s=>s==="pointer").length') == pointer_signals
            assert pixel(30, 30) == [0, 0, 180, 255]
            page.set_viewport_size({'width': 300, 'height': 300})
            page.wait_for_function('probe.data[3]===1')
            page.mouse.move(40, 40)
            pointer_signals = page.evaluate('probe.signals.filter(s=>s==="pointer").length')
            page.mouse.wheel(0, 50)
            page.wait_for_function('probe.data[8]===1&&probe.data[5]===-30')
            assert data()[3] == 0 and data()[9] == 50
            assert page.evaluate('probe.signals.filter(s=>s==="pointer").length') == pointer_signals
            assert pixel(30, 30) == [220, 220, 220, 255]
            assert pixel(150, 25) == [255, 255, 255, 255]
            page.screenshot(path=str(out/'prepaint-scroll.png'))
            assert data()[0] == data()[1] and not errors, (data(), errors)
            browser.close()
    finally: server.shutdown()
    print('Core prepaint WASM/Chromium: unknown pointer, clipping, deferred paint, stationary resize/scroll, passive/modal occlusion PASS')
if __name__ == '__main__': main()
