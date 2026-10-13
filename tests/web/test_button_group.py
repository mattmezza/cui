# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM ButtonGroup input and software-rendered corner/separator/icon pixels."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
from test_button_activation_blocker import HTML as HOST_HTML
ROOT = Path(__file__).resolve().parents[2]
HTML = HOST_HTML.replace("ctx.clearRect(0,0,360,300);let depth=0;", "ctx.putImageData(new ImageData(new Uint8ClampedArray(e.memory.buffer,e.probe_pixels(),360*300*4),360,300),0,0);let depth=0;")
for kind in [0, 1, 5]:
    HTML = HTML.replace(f"else if(c[0]==={kind})", "else if(false)")
HTML = HTML.replace("probe.frames++;", "canvas.style.cursor=e.group_stat(8,0)===e.probe_crosshair()?'crosshair':'default';probe.frames++;")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-group-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','probe_pixels','probe_crosshair','group_stat','group_image_rect']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_group_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page(viewport={'width':480,'height':600},has_touch=True)
            errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe?.frames>=3')
            def stat(field,index=0):return page.evaluate('([f,i])=>probe.e.group_stat(f,i)',[field,index])
            def reset(mode):page.evaluate('(m)=>probe.reset(m)',mode)
            def point(id):
                r=page.evaluate('(id)=>Array.from({length:4},(_,i)=>probe.e.probe_rect(id,i))',id)
                return {'x':10+r[0]+r[2]/2,'y':10+r[1]+r[3]/2,'id':1}
            def mouse(id):
                q=point(id);page.mouse.click(q['x'],q['y'])
            session=page.context.new_cdp_session(page)
            def touch(kind,points):session.send('Input.dispatchTouchEvent',{'type':kind,'touchPoints':points})
            def tap(id):touch('touchStart',[point(id)]);touch('touchEnd',[])
            reset(0);mouse(12);page.wait_for_function('probe.e.group_stat(0,0)===1');assert stat(1)==4
            tap(10);page.wait_for_function('probe.e.group_stat(0,0)===2');assert stat(1)==1 and stat(7)==0
            page.keyboard.press('Enter');assert stat(0)==2  # Source pointer-only callback policy.
            reset(1);mouse(10);page.wait_for_function('probe.e.group_stat(0,0)===1');assert stat(1)==3
            page.keyboard.press('Enter');page.wait_for_function('probe.e.group_stat(0,0)===2');assert stat(1)==2
            assert stat(2,0)==0 and stat(2,1)==0
            reset(2);mouse(10);tap(12);page.wait_for_function('probe.e.group_stat(0,0)===2');assert stat(1)==2
            for mode in [3,4]:
                reset(mode);mouse(10);tap(10);page.wait_for_function('probe.events.at(-1)===2');assert stat(0)==0 and stat(1)==2
                tap(12);page.wait_for_function('probe.e.group_stat(0,0)===1');assert stat(1)==4
            reset(0);touch('touchStart',[point(10)]);touch('touchCancel',[]);page.wait_for_function('probe.events.at(-1)===7');assert stat(0)==0
            tap(10);page.wait_for_function('probe.e.group_stat(0,0)===1');assert stat(1)==1
            reset(6);mouse(10);tap(10);page.wait_for_function('probe.e.group_stat(7,0)===2');assert stat(0)==0
            for mode in [0,5]:
                reset(mode)
                bounds=page.evaluate('Array.from({length:3},(_,j)=>Array.from({length:4},(_,i)=>probe.e.probe_rect(10+j,i)))')
                vertical=mode==5
                for previous,current in zip(bounds,bounds[1:]):
                    assert current[1 if vertical else 0]==previous[1 if vertical else 0]+previous[3 if vertical else 2]
                first,last=bounds[0],bounds[2]
                def pixel(x,y):return page.evaluate('([x,y])=>Array.from(document.querySelector("canvas").getContext("2d").getImageData(x,y,1,1).data)',[int(x),int(y)])
                assert pixel(first[0],first[1])[:3]==[245,245,245]  # Actual rounded outer corner.
                inner=bounds[1]
                assert pixel(inner[0],inner[1])[:3]!=[245,245,245]  # Square shared join.
                # One-pixel separator, not two coincident child borders.
                if vertical:
                    assert pixel(first[0]+first[2]/2,inner[1]-1)[:3]==[10,40,60]
                    assert pixel(first[0]+first[2]/2,inner[1])[:3]==[20,70,100]
                else:
                    assert pixel(inner[0]-1,first[1]+first[3]/2)[:3]==[10,40,60]
                    assert pixel(inner[0],first[1]+first[3]/2)[:3]==[20,70,100]
                # Restrict the check to the actual first IMAGE bounds, excluding labels.
                white=page.evaluate('()=>{let c=document.querySelector("canvas").getContext("2d"),n=0,r=Array.from({length:4},(_,i)=>probe.e.group_image_rect(i));if(r.some(v=>v<0))throw Error("missing image command");const p=c.getImageData(...r).data;for(let i=0;i<p.length;i+=4)if(p[i]>230&&p[i+1]>230&&p[i+2]>230&&p[i+3]>0)n++;return n;}')
                assert white>3, ('missing check SVG pixels',mode,white)
                mouse(10);page.wait_for_function('document.querySelector("canvas").style.cursor==="crosshair"')
                page.screenshot(path=str(out/f'group-{mode}.png'))
            assert not errors,errors;browser.close()
        print('ButtonGroup real mouse/touch/keyboard controlled states and software corner/join/SVG pixels PASS')
    finally:server.shutdown()

if __name__=='__main__':main()
