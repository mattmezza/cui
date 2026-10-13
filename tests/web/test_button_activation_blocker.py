# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual nested Button activation barriers, Core capture and scroll, driven by Chromium input."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><style>body{margin:0;width:1600px;height:2000px}canvas{position:fixed;top:10px;left:10px;touch-action:none;outline:none}</style><canvas tabindex="0" width="360" height="300"></canvas><script>
(async()=>{
 const m=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer()),imports={};
 for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
 const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();
 const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d'),touches=new Map();
 window.probe={e,frames:0,events:[],fallbacks:0};
 function frame(){if(e.probe_frame())throw Error('frame');ctx.clearRect(0,0,360,300);let depth=0;
 for(let i=0;i<e.probe_count();i++){const c=Array.from({length:10},(_,j)=>e.probe_command(i,j));
 if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
 else if(c[0]===4){ctx.restore();depth--;}
 else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
 else if(c[0]===1){ctx.strokeStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.strokeRect(...c.slice(1,5));}
 else if(c[0]===2){ctx.font=`${c[9]}px sans-serif`;ctx.fillStyle=`rgb(${c[5]},${c[6]},${c[7]})`;ctx.textBaseline='top';ctx.fillText(new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_text_length(i))),c[1],c[2]);}
 else if(c[0]===5){const b=Array.from({length:9},(_,f)=>e.probe_bitmap(i,f));if(!b[0])throw Error('bitmap');const raw=new Uint8Array(e.memory.buffer,b[0],b[3]*b[2]),pixels=new Uint8ClampedArray(b[1]*b[2]*4);for(let y=0;y<b[2];y++)for(let x=0;x<b[1];x++)for(let k=0;k<4;k++)pixels[(y*b[1]+x)*4+k]=raw[y*b[3]+x*4+k]*b[4+k]/255;const image=document.createElement('canvas');image.width=b[1];image.height=b[2];image.getContext('2d').putImageData(new ImageData(pixels,b[1],b[2]),0,0);ctx.save();ctx.globalAlpha=b[8];ctx.drawImage(image,...c.slice(1,5));ctx.restore();}}

 if(depth)throw Error('clip');probe.frames++;probe.data=Array.from({length:6},(_,i)=>e.probe_stat(i));}
 probe.reset=(mode)=>{touches.clear();if(e.probe_init(mode))throw Error('init');window.scrollTo(0,0);probe.fallbacks=0;frame();frame();frame();};
 function queue(type,ev,dx=0,dy=0){const r=canvas.getBoundingClientRect();if(!e.probe_event(type,ev.clientX-r.left,ev.clientY-r.top,ev.pointerId||0,ev.pointerType==='touch'?1:0,dy))throw Error('queue');probe.events.push(type);frame();}
 canvas.addEventListener('wheel',ev=>{ev.preventDefault();let dx=ev.deltaX,dy=ev.deltaY;if(ev.shiftKey&&dx===0){dx=dy;dy=0;}queue(6,ev,-dx,-dy);if(!probe.data[4]){scrollBy(dx,dy);probe.fallbacks++;}},{passive:false});
 canvas.addEventListener('pointerdown',ev=>{canvas.setPointerCapture(ev.pointerId);if(ev.pointerType==='touch')touches.set(ev.pointerId,{x:ev.clientX,y:ev.clientY,owned:false});queue(1,ev);});
 canvas.addEventListener('pointermove',ev=>{const t=touches.get(ev.pointerId);if(!t){queue(0,ev);return;}const dx=ev.clientX-t.x,dy=ev.clientY-t.y;t.x=ev.clientX;t.y=ev.clientY;if(t.owned){scrollBy(-dx,-dy);return;}queue(0,ev);if(e.probe_page_available(ev.pointerId)){queue(7,ev);e.probe_page_end(ev.pointerId);t.owned=true;scrollBy(-dx,-dy);probe.fallbacks++;}});
 canvas.addEventListener('pointerup',ev=>{const t=touches.get(ev.pointerId);if(!t?.owned)queue(2,ev);touches.delete(ev.pointerId);e.probe_page_end(ev.pointerId);});
 canvas.addEventListener('pointercancel',ev=>{if(!touches.get(ev.pointerId)?.owned)queue(7,ev);touches.delete(ev.pointerId);e.probe_page_end(ev.pointerId);});
 canvas.addEventListener('keydown',ev=>{const key={Tab:1,Enter:2,' ':3,Escape:4}[ev.key];if(key){ev.preventDefault();e.probe_key(key,0,Number(ev.repeat));frame();}});
 canvas.addEventListener('keyup',ev=>{const key={Tab:1,Enter:2,' ':3,Escape:4}[ev.key];if(key){ev.preventDefault();e.probe_key(key,1,0);frame();}});
 probe.reset(1);
})().catch(error=>{window.probeError=String(error);throw error;});</script>'''
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-blocker-probe')
    parser.add_argument('--zig', default='zig')
    parser.add_argument('--case', choices=['all', 'mouse', 'touch'], default='all')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output([args.zig, 'version'], text=True).strip() == '0.16.0'
    source_root=args.source_root.resolve()
    tree = ast.parse((source_root/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in node.targets))
    sources = [str(source_root/s) for s in sources if s.startswith('src/')]
    exports = ['probe_init', 'probe_event', 'probe_key', 'probe_frame', 'probe_page_available', 'probe_page_end', 'probe_stat', 'probe_rect', 'probe_count', 'probe_command', 'probe_text', 'probe_text_length', 'probe_bitmap']
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(source_root/'include'), '-I', str(source_root/'third_party/clay'), str(ROOT/'tests/web/button_blocker_probe.c'), *sources, '-lm', '-Wl,-z,stack-size=8388608', *[f'-Wl,--export={s}' for s in exports], '-o', str(out/'probe.wasm')], check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(out)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=args.chromium)
            page = browser.new_page(viewport={'width': 480, 'height': 600}, has_touch=True)
            errors = []; page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}'); page.wait_for_function('window.probe?.frames>=3')
            def data(): return page.evaluate('probe.data')
            def reset(mode):
                page.evaluate('(mode)=>probe.reset(mode)', mode)
                page.wait_for_function('scrollX===0&&scrollY===0')
            def point(owner):
                b=page.evaluate('(id)=>Array.from({length:4},(_,i)=>probe.e.probe_rect(id,i))',owner)
                return {'x':10+b[0]+b[2]/2,'y':10+b[1]+b[3]/2,'id':1}
            session=page.context.new_cdp_session(page)
            def touch(kind, points): session.send('Input.dispatchTouchEvent', {'type':kind,'touchPoints':points})
            def tap(owner): touch('touchStart',[point(owner)]); touch('touchEnd',[])
            for mode in [1,2]:
                if args.case != 'touch':
                    reset(mode); editor=point(90); page.mouse.click(editor['x'],editor['y'])
                    page.wait_for_function('probe.data[2]===90'); child=point(20)
                    page.mouse.click(child['x'],child['y']); page.wait_for_function('probe.events.at(-1)===2')
                    assert data()[0:3]==[0,0,90], ('inert mouse parent leak',mode,data())
                    page.keyboard.press('Tab'); page.keyboard.press('Enter')
                    page.wait_for_function('probe.data[0]===1'); assert data()[1]==0 and data()[2]==10, data()
                if args.case != 'mouse':
                    reset(mode); tap(90); page.wait_for_function('probe.data[2]===90')
                    tap(20); page.wait_for_function('probe.events.at(-1)===2')
                    assert data()[0:3]==[0,0,90], ('inert touch parent leak',mode,data())
            reset(0); child=point(20); page.mouse.click(child['x'],child['y'])
            page.wait_for_function('probe.data[1]===1'); assert data()[0]==0
            tap(20); page.wait_for_function('probe.data[1]===2'); assert data()[0]==0
            # Existing capture keeps moves and UP for cleanup, but release over an inert child cannot click parent.
            reset(1); parent=point(10); child=point(20)
            page.mouse.move(parent['x'],parent['y']); page.mouse.down()
            page.wait_for_function('probe.data[5]===1'); page.mouse.move(child['x'],child['y'])
            assert data()[5]==1; page.mouse.up(); page.wait_for_function('probe.data[5]===0')
            assert data()[0:2]==[0,0], data()
            # Scroll from the blocked child still belongs to its ancestor, not the page.
            for mode in [1,2]:
                reset(mode); child=point(20); page.mouse.move(child['x'],child['y']); page.mouse.wheel(0,20)
                page.wait_for_function('probe.data[3]===20'); assert data()[0:2]==[0,0] and page.evaluate('probe.fallbacks')==0
                reset(mode); child=point(20); touch('touchStart',[child])
                touch('touchMove',[{**child,'y':child['y']-30}]); page.wait_for_function('probe.data[3]===30')
                touch('touchEnd',[]); assert data()[0:2]==[0,0] and page.evaluate('[scrollY,probe.fallbacks]')==[0,0], data()
            reset(1); touch('touchStart',[point(20)]); touch('touchCancel',[])
            page.wait_for_function('probe.events.at(-1)===7'); assert data()[0:2]==[0,0]
            # A later ordinary parent tap is reachable after cancellation.
            tap(10); page.wait_for_function('probe.data[0]===1'); assert data()[1]==0
            reset(0); touch('touchStart',[point(20)]); touch('touchCancel',[])
            page.wait_for_function('probe.events.at(-1)===7'); assert data()[0:2]==[0,0]
            tap(20); page.wait_for_function('probe.data[1]===1'); assert data()[0]==0
            page.screenshot(path=str(out/'button-blocker.png')); assert not errors, errors
            browser.close()
    finally: server.shutdown()
    print('Button blocker: inert nested mouse/touch, focus/keyboard/capture, ancestor wheel/pan and cancellation PASS')
if __name__ == '__main__': main()
