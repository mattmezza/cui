# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual Chromium pointer enter/leave, capture and blur with Core WASM pixels."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><style>body{margin:0}canvas{display:block;margin:10px}</style>
<canvas width="240" height="160" tabindex="0"></canvas><button id="outside">Outside focus</button><script>
(async()=>{
 const module=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer());
 const imports={};for(const i of WebAssembly.Module.imports(module)){(imports[i.module]??={})[i.name]=i.name==='proc_exit'?()=>{throw Error('WASI exit')}:()=>0;}
 const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();if(e.presence_init())throw Error('init');
 const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');window.presence={e,frames:0,history:[],signals:[],active:null};let suspended=false;
 const frame=()=>{
  if(e.presence_frame())throw Error('frame');ctx.clearRect(0,0,240,160);let depth=0;
  for(let i=0;i<e.presence_count();i++){
   const c=Array.from({length:9},(_,j)=>e.presence_command(i,j));
   if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
   else if(c[0]===4){if(!depth)throw Error('clip underflow');ctx.restore();depth--;}
   else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
   else throw Error('unexpected command');
  }
  if(depth)throw Error('clip imbalance');presence.frames++;presence.data=Array.from({length:15},(_,i)=>e.presence_stat(i));presence.history.push(presence.data);
 };
 const queue=(type,x=0,y=0)=>{if(!e.presence_event(type,x,y))throw Error('queue');};
 const leave=()=>queue(15);
 const outside=(ev,r)=>ev.clientX<r.left||ev.clientX>=r.right||ev.clientY<r.top||ev.clientY>=r.bottom;
 canvas.addEventListener('pointerenter',()=>{presence.signals.push('enter');queue(14);frame();});
 canvas.addEventListener('pointerleave',()=>{presence.signals.push('leave');leave();frame();});
 for(const [name,type] of [['pointermove',0],['pointerdown',1],['pointerup',2]])canvas.addEventListener(name,ev=>{
  const r=canvas.getBoundingClientRect();presence.signals.push(name);
  if(type===1){suspended=false;presence.active=ev.pointerId;canvas.focus();canvas.setPointerCapture(ev.pointerId);}
  if(type===2&&presence.active===ev.pointerId)presence.active=null;
  queue(type,ev.clientX-r.left,ev.clientY-r.top);if(outside(ev,r))leave();frame();
 });
 const cancel=ev=>{presence.signals.push(ev.type);if(presence.active!==ev.pointerId)return;presence.active=null;queue(7);frame();};
 canvas.addEventListener('gotpointercapture',()=>presence.signals.push('gotpointercapture'));
 canvas.addEventListener('lostpointercapture',cancel);canvas.addEventListener('pointercancel',cancel);
 const focusLoss=()=>{presence.signals.push('blur');presence.active=null;if(!suspended){queue(7);suspended=true;}leave();frame();};
 canvas.addEventListener('focus',()=>{suspended=false;});window.addEventListener('focus',()=>{suspended=false;presence.signals.push('focus');});canvas.addEventListener('blur',focusLoss);window.addEventListener('blur',focusLoss);
 frame();
})().catch(error=>{window.presenceError=String(error);throw error;});
</script>'''
def serve(directory):
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(directory)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server, f'http://127.0.0.1:{server.server_port}'
def fixture(browser, url, out):
    context = browser.new_context(viewport={'width': 340, 'height': 300})
    page = context.new_page()
    errors = []; page.on('pageerror', lambda error: errors.append(str(error)))
    page.set_default_timeout(10000)
    page.goto(url); page.wait_for_function('window.presence?.frames===1')
    def data(): return page.evaluate('presence.data')
    def pixel(x, y): return page.evaluate('([x,y])=>Array.from(document.querySelector("canvas").getContext("2d").getImageData(x,y,1,1).data)', [x, y])
    assert data()[2:5] == [0, 0, 0] and data()[10] == 0
    assert pixel(30, 30) == [0, 0, 180, 255]
    page.mouse.move(45, 45); page.wait_for_function('presence.data[4]===1')
    assert page.evaluate('presence.history.some(d=>d[2]===1&&d[3]===0)'), 'First ENTER fabricated a position'
    assert data()[2:5] == [1, 1, 1] and data()[5:7] == [35, 35]
    assert pixel(30, 30) == [0, 180, 0, 255] and pixel(150, 25) == [240, 200, 0, 255]
    page.mouse.move(300, 45); page.wait_for_function('presence.data[2]===0')
    assert data()[3:5] == [0, 0] and data()[5:7] == [35, 35] and data()[10] == 0
    assert pixel(30, 30) == [0, 0, 180, 255] and pixel(150, 25) == [255, 255, 255, 255]
    page.mouse.move(45, 45); page.wait_for_function('presence.data[4]===1')
    page.mouse.down(); page.wait_for_function('presence.data[7]===20')
    page.mouse.move(300, 45); page.wait_for_function('presence.data[2]===0&&presence.data[5]===290')
    assert data()[7:9] == [20, 20] and data()[10] == 7 and data()[11] == 0
    assert pixel(30, 30) == [0, 0, 180, 255]
    page.mouse.up(); page.wait_for_function('presence.data[7]===0')
    assert data()[2:5] == [0, 0, 0] and data()[10:12] == [0, 0]
    page.mouse.move(45, 45); page.wait_for_function('presence.data[4]===1')
    page.mouse.down(); page.wait_for_function('presence.data[7]===20')
    page.mouse.move(46, 45)
    page.wait_for_function('presence.signals.filter(s=>s==="gotpointercapture").length>=2')
    cancel_count = data()[12]
    page.evaluate('document.querySelector("canvas").releasePointerCapture(presence.active)')
    # The browser dispatches lostpointercapture at the next real pointer update.
    page.mouse.move(47, 45)
    page.wait_for_function('(n)=>presence.data[7]===0&&presence.data[12]===n+1', arg=cancel_count)
    assert data()[2:5] == [1, 1, 1] and data()[8] == 20 and data()[11] == 0
    page.mouse.up(); page.wait_for_timeout(80); assert data()[11] == 0
    page.mouse.down(); page.wait_for_function('presence.data[7]===20')
    cancel_count = data()[12]
    page.keyboard.press('Tab')  # Real focus transition blurs the focused canvas.
    page.wait_for_function('(n)=>presence.data[2]===0&&presence.data[7]===0&&presence.data[12]===n+1', arg=cancel_count)
    assert data()[5:7] == [37, 35] and data()[11] == 0
    assert pixel(30, 30) == [0, 0, 180, 255] and pixel(150, 25) == [255, 255, 255, 255]
    page.mouse.up(); page.wait_for_timeout(80); assert data()[11] == 0
    page.mouse.move(48, 45); page.wait_for_function('presence.data[4]===1')
    page.mouse.click(48, 45); page.wait_for_function('presence.data[11]===1')
    page.wait_for_timeout(120); assert data()[11] == 1 and data()[0] == data()[1]
    # Distinct real focus transitions rearm cancellation without fabricating presence.
    for _ in range(2):
        cancel_count=data()[12]
        page.locator('canvas').focus()
        page.keyboard.press('Tab')
        page.wait_for_function('(n)=>presence.data[2]===0&&presence.data[12]===n+1',arg=cancel_count)
        assert data()[3:5]==[0,0] and data()[7]==0 and data()[11]==1
    page.screenshot(path=str(out/'pointer-presence.png'))
    assert not errors, errors; context.close()
def gallery(browser, url):
    from test_touch_components import Audit
    audit = Audit(browser, url); audit.route('Button')
    page = audit.page
    def target(): return audit.target('Run action', 1)
    def clicks(): return page.evaluate('cuiGallery.exports.gallery_clicks()')
    def fill():
        return page.evaluate('''b=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===0&&[1,2,3,4].every((f,j)=>Math.abs(e.gallery_command(i,f)-b[j])<.5))return [5,6,7,8].map(f=>e.gallery_command(i,f));return null;}''', target()['f'][1:5])
    page.mouse.move(5, 5); page.wait_for_timeout(100); normal = fill()
    point = audit.point(target()); page.mouse.move(point['x'], point['y']); page.wait_for_timeout(100)
    assert fill() != normal and page.locator('canvas').evaluate('e=>getComputedStyle(e).cursor') == 'pointer'
    rect = page.locator('canvas').bounding_box(); outside = {'x': rect['x']+rect['width']+12, 'y': point['y']}
    page.mouse.move(outside['x'], outside['y']); page.wait_for_timeout(100)
    assert fill() == normal and page.locator('canvas').evaluate('e=>getComputedStyle(e).cursor') in ['auto', 'default']
    page.mouse.move(point['x'], point['y']); page.mouse.down(); page.wait_for_timeout(80)
    count = clicks(); page.mouse.move(outside['x'], outside['y']); page.wait_for_timeout(100)
    assert page.locator('canvas').evaluate('e=>getComputedStyle(e).cursor') == 'pointer', 'Capture cursor lost on leave'
    page.mouse.up(); page.wait_for_timeout(100); assert clicks() == count
    page.evaluate('''()=>{window.presenceGalleryPointer=null;document.querySelector('canvas').addEventListener('gotpointercapture',e=>{window.presenceGalleryPointer=e.pointerId;},{once:true});}''')
    page.mouse.move(point['x'], point['y']);page.mouse.down();page.mouse.move(point['x']+1,point['y'])
    page.wait_for_function('window.presenceGalleryPointer!==null')
    page.evaluate('document.querySelector("canvas").releasePointerCapture(presenceGalleryPointer)')
    page.mouse.move(point['x']+2,point['y']);page.wait_for_timeout(100)
    page.mouse.up();page.wait_for_timeout(100);assert clicks()==count,'Actual gallery lostcapture ghost-activated'
    page.mouse.move(point['x'], point['y']); page.mouse.down(); page.wait_for_timeout(80)
    page.evaluate('''()=>{const c=document.querySelector('canvas');c.focus();c.blur();}''')
    page.wait_for_timeout(100); assert fill() == normal
    page.mouse.up(); page.wait_for_timeout(100); assert clicks() == count
    page.mouse.move(point['x']+1, point['y']); page.mouse.click(point['x']+1, point['y'])
    page.wait_for_function('(n)=>cuiGallery.exports.gallery_clicks()===n+1', arg=count)
    assert not audit.errors, audit.errors; page.close()
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/pointer-presence-probe')
    parser.add_argument('--directory', type=Path, help='Also require actual gallery host regression against this built directory')
    parser.add_argument('--zig', default='zig')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output([args.zig, 'version'], text=True).strip() == '0.16.0'
    exports = ['presence_init', 'presence_event', 'presence_frame', 'presence_stat', 'presence_count', 'presence_command']
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-I', str(ROOT/'include'), '-I', str(ROOT/'third_party/clay'), str(ROOT/'tests/web/pointer_presence_probe.c'), str(ROOT/'src/core/cui.c'), '-lm',
        '-Wl,-z,stack-size=8388608', *[f'-Wl,--export={s}' for s in exports], '-o', str(out/'probe.wasm')], check=True)
    (out/'index.html').write_text(HTML); server, url = serve(out)
    gallery_server = None
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=args.chromium)
            fixture(browser, url, out)
            if args.directory:
                assert (args.directory/'gallery.wasm').is_file()
                gallery_server, gallery_url = serve(args.directory.resolve()); gallery(browser, gallery_url)
            browser.close()
    finally:
        server.shutdown()
        if gallery_server: gallery_server.shutdown()
    print('Pointer presence WASM/Chromium enter/leave, captured outside motion, lostcapture, blur and next click PASS'+ ('; actual gallery host PASS' if args.directory else ''))
if __name__ == '__main__': main()
