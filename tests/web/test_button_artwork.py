# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM typed Button artwork and loading replacement with real input."""
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
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-artwork-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','artwork_config','artwork_stat']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_artwork_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
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
            pixels=lambda: page.evaluate("Array.from(document.querySelector('canvas').getContext('2d').getImageData(0,0,360,300).data)")
            def config(mode):
                page.evaluate('(n)=>{probe.e.artwork_config(n);probe.step()}',mode)
            def colored(data, rgb):
                return sum(1 for i in range(0,len(data),4) if data[i:i+3]==rgb)
            def red(data):
                return sum(1 for i in range(0,len(data),4) if data[i]>data[i+1]+80 and data[i+2]>data[i+1]+20)
            def green(data):
                return sum(1 for i in range(0,len(data),4) if data[i+1]>data[i]+80 and data[i+1]>data[i+2]+80)
            normal=pixels()
            assert colored(normal,[231,24,91])>200, 'custom inline SVG must paint'
            rect=page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')
            x=rect[0]+rect[2]/2+10;y=rect[1]+rect[3]/2+10
            page.mouse.click(x,y);page.wait_for_function('probe.data[1]===1')
            cdp=page.context.new_cdp_session(page)
            cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':1}]})
            cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            page.wait_for_function('probe.data[1]===2')
            config(1)
            assert colored(pixels(),[231,24,91])==0, 'ordinary Icon replaced while loading'
            config(2)
            assert red(pixels())>100, 'custom loading Icon must paint'
            config(3)
            assert green(pixels())>30, 'ProgressCircle must survive loading'
            assert colored(pixels(),[231,24,91])==0, 'loading override must not replace Progress'
            config(4)
            assert red(pixels())>100, 'explicit Spinner must survive loading'
            before=page.evaluate('probe.data[1]')
            page.mouse.click(x,y)
            cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':2}]})
            cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            page.wait_for_timeout(80)
            assert page.evaluate('probe.data[1]')==before, 'loading remains nonactivatable'
            config(5)
            assert page.evaluate('probe.e.artwork_stat(0)')==2
            assert colored(pixels(),[10,180,30])==0, 'failed progress buffer paints no stale image'
            resource=page.evaluate('probe.e.artwork_stat(1)')
            config(6)
            assert page.evaluate('probe.e.probe_count()')==0
            assert page.evaluate('probe.e.artwork_stat(1)')==resource
            assert page.evaluate('probe.e.artwork_stat(3)')==0
            config(7)
            assert colored(pixels(),[231,24,91])>200, 'inert still paints artwork'
            page.mouse.click(x,y);page.wait_for_timeout(60)
            assert page.evaluate('probe.data[1]')==before
            config(0)
            page.mouse.click(x,y);page.wait_for_function('(n)=>probe.data[1]===n+1',arg=before)
            assert not errors,errors
            browser.close()
        print('PASS: typed SVG/loading/Spinner/Progress pixels, actual mouse/CDP touch, passive and failure lifecycle')
    finally:server.shutdown()
if __name__=='__main__':main()
