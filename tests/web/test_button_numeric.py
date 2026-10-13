# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM numeric Button sizing, open presentation and real input."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
from test_button_activation_blocker import HTML as HOST_HTML
HTML = HOST_HTML.replace("probe.reset=(mode)=>", "probe.step=frame;probe.reset=(mode)=>")
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-numeric-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','numeric_config']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_numeric_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page(viewport={'width':480,'height':600},has_touch=True)
            errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe?.frames>=3')
            page.mouse.move(450,500)
            normal_pixels=page.evaluate("Array.from(document.querySelector('canvas').getContext('2d').getImageData(0,0,360,300).data)")
            normal=page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')
            assert 0<normal[3]<40,normal
            page.evaluate('probe.e.numeric_config(1);probe.step()')
            square=page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')
            assert square[2:]==[80,80],square
            page.evaluate('probe.e.numeric_config(0);probe.step()')
            x=normal[0]+normal[2]/2+10;y=normal[1]+normal[3]/2+10
            page.mouse.click(x,y);page.wait_for_function('probe.data[1]===1')
            cdp=page.context.new_cdp_session(page)
            cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':1}]})
            cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            page.wait_for_function('probe.data[1]===2')
            for mode in [3,4]:
                page.evaluate('(n)=>{probe.e.numeric_config(n);probe.step()}',mode)
                page.mouse.click(x,y)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':2}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_timeout(80)
                assert page.evaluate('probe.data[1]')==2
            page.evaluate('probe.e.numeric_config(2);probe.step()')
            page.mouse.move(450,500);page.evaluate('probe.step()')
            opened_pixels=page.evaluate("Array.from(document.querySelector('canvas').getContext('2d').getImageData(0,0,360,300).data)")
            assert normal_pixels!=opened_pixels,'open must change actual pixels'
            # Command enum is queried by resource validity, independent of enum numbering.
            caret=page.evaluate("Array.from({length:probe.e.probe_count()},(_,i)=>i).filter(i=>probe.e.probe_bitmap(i,0)>0 && probe.e.probe_command(i,3)===16)")
            assert len(caret)==1,caret
            assert not errors,errors
            browser.close()
        print('PASS: numeric FIT/square geometry, actual mouse/CDP touch, disabled/loading and open paint')
    finally:server.shutdown()
if __name__=='__main__':main()
