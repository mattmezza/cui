# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual styled Tabs, Core routing and existing page-host policy, driven by Chromium input."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><style>body{margin:0;width:1600px;height:2000px}canvas{position:fixed;top:10px;left:10px;touch-action:none}</style><canvas width="360" height="260"></canvas><script>
(async()=>{
 const m=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer()),imports={};
 for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
 const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();
 const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d'),touches=new Map();
 window.probe={e,frames:0,events:[],fallbacks:0};
 function frame(){if(e.probe_frame())throw Error('frame');ctx.clearRect(0,0,360,260);let depth=0;
 for(let i=0;i<e.probe_count();i++){const c=Array.from({length:10},(_,j)=>e.probe_command(i,j));
 if(c[0]===3){ctx.save();depth++;ctx.beginPath();ctx.rect(...c.slice(1,5));ctx.clip();}
 else if(c[0]===4){ctx.restore();depth--;}
 else if(c[0]===0){ctx.fillStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.fillRect(...c.slice(1,5));}
 else if(c[0]===1){ctx.strokeStyle=`rgba(${c[5]},${c[6]},${c[7]},${c[8]/255})`;ctx.strokeRect(...c.slice(1,5));}
 else if(c[0]===2){ctx.font=`${c[9]}px sans-serif`;ctx.fillStyle=`rgb(${c[5]},${c[6]},${c[7]})`;ctx.textBaseline='top';ctx.fillText(new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_text_length(i))),c[1],c[2]);}}
 if(depth)throw Error('clip');probe.frames++;probe.data=Array.from({length:14},(_,i)=>e.probe_stat(i));}
 probe.reset=(axis,disabled=0)=>{touches.clear();if(e.probe_init(axis,disabled))throw Error('init');window.scrollTo(0,0);probe.fallbacks=0;frame();frame();frame();};
 function queue(type,ev,dx=0,dy=0){const r=canvas.getBoundingClientRect();if(!e.probe_event(type,ev.clientX-r.left,ev.clientY-r.top,dx,dy,ev.pointerId||0,ev.pointerType==='touch'?1:0))throw Error('queue');probe.events.push(type);frame();}
 canvas.addEventListener('wheel',ev=>{ev.preventDefault();let dx=ev.deltaX,dy=ev.deltaY;if(ev.shiftKey&&dx===0){dx=dy;dy=0;}queue(6,ev,-dx,-dy);if(!probe.data[3]){scrollBy(dx,dy);probe.fallbacks++;}},{passive:false});
 canvas.addEventListener('pointerdown',ev=>{canvas.setPointerCapture(ev.pointerId);if(ev.pointerType==='touch')touches.set(ev.pointerId,{x:ev.clientX,y:ev.clientY,owned:false});queue(1,ev);});
 canvas.addEventListener('pointermove',ev=>{const t=touches.get(ev.pointerId);if(!t){queue(0,ev);return;}const dx=ev.clientX-t.x,dy=ev.clientY-t.y;t.x=ev.clientX;t.y=ev.clientY;if(t.owned){scrollBy(-dx,-dy);return;}queue(0,ev);if(e.probe_page_available(ev.pointerId)){queue(7,ev);e.probe_page_end(ev.pointerId);t.owned=true;scrollBy(-dx,-dy);probe.fallbacks++;}});
 canvas.addEventListener('pointerup',ev=>{const t=touches.get(ev.pointerId);if(!t?.owned)queue(2,ev);touches.delete(ev.pointerId);e.probe_page_end(ev.pointerId);});
 canvas.addEventListener('pointercancel',ev=>{if(!touches.get(ev.pointerId)?.owned)queue(7,ev);touches.delete(ev.pointerId);e.probe_page_end(ev.pointerId);});
 probe.reset(1);
})().catch(error=>{window.probeError=String(error);throw error;});</script>'''
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/tabs-scroll-probe')
    parser.add_argument('--zig', default='zig')
    parser.add_argument('--case', choices=['all', 'touch'], default='all')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output([args.zig, 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(node.value) for node in tree.body if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in node.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    exports = ['probe_init', 'probe_event', 'probe_frame', 'probe_page_available', 'probe_page_end', 'probe_stat', 'probe_count', 'probe_command', 'probe_text', 'probe_text_length']
    subprocess.run([args.zig, 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(ROOT/'include'), '-I', str(ROOT/'third_party/clay'), str(ROOT/'tests/web/tabs_scroll_probe.c'), *sources, '-lm', '-Wl,-z,stack-size=8388608', *[f'-Wl,--export={s}' for s in exports], '-o', str(out/'probe.wasm')], check=True)
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
            def reset(axis, disabled=0): page.evaluate('([a,d])=>probe.reset(a,d)', [axis, disabled]); page.wait_for_function('scrollX===0&&scrollY===0')
            def point():
                d=data(); return (20+d[5], 20+d[6])
            for axis in ([0, 1] if args.case != 'touch' else []):
                reset(axis); x,y=point(); page.mouse.move(x,y)
                before=data()[0]; position=data()[9+axis]
                page.evaluate('probe.pixels=document.querySelector("canvas").getContext("2d").getImageData(0,0,360,260).data')
                page.mouse.wheel(0 if axis else 20,20 if axis else 0)
                page.wait_for_function('probe.data[0]>0')
                assert data()[0]==before+20 and data()[2]==0 and data()[3]==1, (data(), page.evaluate('[scrollX,scrollY,probe.fallbacks]'))
                assert abs(data()[9+axis]-(position-20))<.001, data()
                assert page.evaluate('(()=>{const now=document.querySelector("canvas").getContext("2d").getImageData(0,0,360,260).data;let changed=0;for(let i=0;i<now.length;i++)changed+=now[i]!==probe.pixels[i];return changed;})()')>100
                assert page.evaluate('[scrollX,scrollY,probe.fallbacks]')==[0,0,0], data()
                page.mouse.wheel(0 if axis else 10000,10000 if axis else 0)
                page.wait_for_function('probe.data[0]===probe.data[1]')
                assert data()[2]==0 and page.evaluate('probe.fallbacks')==0
                page.mouse.wheel(0 if axis else 15,15 if axis else 0)
                page.wait_for_function('probe.data[2]===15')
                assert page.evaluate('[scrollX,scrollY,probe.fallbacks]')==[0,0,0]
            if args.case != 'touch':
                reset(0); x,y=point(); page.mouse.move(x,y); page.keyboard.down('Shift')
                page.mouse.wheel(0,20); page.keyboard.up('Shift')
                page.wait_for_function('probe.data[0]===20')
                assert data()[2]==0 and page.evaluate('[scrollX,scrollY,probe.fallbacks]')==[0,0,0]
            reset(1); x,y=point(); session=page.context.new_cdp_session(page)
            def touch(kind, points): session.send('Input.dispatchTouchEvent', {'type':kind,'touchPoints':points})
            touch('touchStart',[{'x':x+30,'y':y+60,'id':1}]); touch('touchMove',[{'x':x+30,'y':y+30,'id':1}])
            page.wait_for_function('probe.data[0]>0')
            assert data()[0]==30 and data()[2]==0 and data()[3]==1, (data(), page.evaluate('[scrollX,scrollY,probe.fallbacks]'))
            assert page.evaluate('[scrollY,probe.fallbacks]')==[0,0], data()
            touch('touchEnd',[]); assert data()[4]==10, data()
            # Repeated fresh contacts reach the inner boundary, then the parent.
            for _ in range(12):
                x,y=point(); touch('touchStart',[{'x':x+30,'y':y+60,'id':2}]); touch('touchMove',[{'x':x+30,'y':y+20,'id':2}]); touch('touchEnd',[])
                if data()[2]>0: break
            page.wait_for_function('probe.data[2]>0'); assert data()[0]==data()[1] and page.evaluate('probe.fallbacks')==0, data()
            reset(1,1); x,y=point(); page.mouse.move(x,y); page.mouse.wheel(0,20)
            page.wait_for_function('probe.data[2]===20'); assert data()[0]==0 and page.evaluate('probe.fallbacks')==0
            reset(1,1); x,y=point(); touch('touchStart',[{'x':x+30,'y':y+60,'id':5}]); touch('touchMove',[{'x':x+30,'y':y+30,'id':5}]); touch('touchEnd',[])
            page.wait_for_function('probe.data[2]===30'); assert data()[0]==0 and data()[4]==10 and data()[11]==0 and page.evaluate('probe.fallbacks')==0
            reset(1); x,y=point(); touch('touchStart',[{'x':x+30,'y':y+60,'id':3}]); touch('touchMove',[{'x':x+30,'y':y+30,'id':3}]); touch('touchCancel',[])
            page.wait_for_function('probe.events.at(-1)===7')
            assert data()[4]==10 and data()[11]==0 and page.evaluate('probe.fallbacks')==0
            reset(1); d=data(); tap={'x':10+d[12], 'y':10+d[13], 'id':4}
            touch('touchStart',[tap]); touch('touchEnd',[])
            page.wait_for_function('probe.data[4]===20')
            assert data()[11]==1 and page.evaluate('probe.fallbacks')==0, data()
            page.screenshot(path=str(out/'tabs-scroll.png')); assert not errors, errors
            browser.close()
    finally: server.shutdown()
    print('Tabs: actual wheel/touch ownership, inner-to-parent boundary, disabled and cancel PASS')
if __name__ == '__main__': main()
