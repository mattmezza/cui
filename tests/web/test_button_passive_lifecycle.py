# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM passive Button caller-state lifecycle and real input reuse."""
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
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-passive-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','passive_scope','passive_stat','passive_unchanged','passive_loading','passive_redraw']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_passive_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
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
            for scope in [1,2]:
                page.evaluate('probe.reset(0)');page.mouse.move(30,25)
                page.mouse.move(31,25);page.mouse.move(32,25)
                page.wait_for_function('probe.e.passive_stat(1)===1 && probe.e.passive_stat(2)===1')
                baseline=page.evaluate('Array.from({length:5},(_,i)=>probe.e.passive_stat(i))')
                count=page.evaluate('probe.data[1]')
                page.evaluate('(s)=>probe.e.passive_scope(s)',scope)
                page.mouse.move(280,150);page.mouse.click(30,25)
                cdp=page.context.new_cdp_session(page)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':30,'y':25,'id':1}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                assert page.evaluate('probe.e.passive_unchanged()')==1
                assert page.evaluate('Array.from({length:5},(_,i)=>probe.e.passive_stat(i))')==baseline
                assert page.evaluate('probe.data[1]')==count
                draw_count=page.evaluate('probe.e.probe_count()')
                assert (draw_count>0 if scope==1 else draw_count==0),(scope,draw_count,page.evaluate('Array.from({length:probe.e.probe_count()},(_,i)=>Array.from({length:5},(_,f)=>probe.e.probe_command(i,f)))'))
                pixels=page.evaluate("""()=>document.querySelector('canvas').getContext('2d').getImageData(0,0,360,300).data.filter((_,i)=>i%4===3).some(v=>v>0)""")
                assert pixels==(scope==1)
                page.evaluate('probe.e.passive_scope(0)');page.mouse.move(29,25)
                page.mouse.click(30,25);page.wait_for_function('(n)=>probe.data[1]===n+1',arg=count)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':30,'y':25,'id':2}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_function('(n)=>probe.data[1]===n+2',arg=count)
                page.keyboard.press('Tab');page.keyboard.press('Enter')
                page.wait_for_function('(n)=>probe.data[1]===n+3',arg=count)
            # A captured touch ending after suppression must not activate or leave a ghost.
            for scope in [1,2]:
                page.evaluate('probe.reset(0)')
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':30,'y':25,'id':3}]})
                page.wait_for_function('probe.events.at(-1)===1')
                page.evaluate('(s)=>probe.e.passive_scope(s)',scope)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_function('probe.events.at(-1)===2')
                assert page.evaluate('probe.e.passive_unchanged()')==1
                assert page.evaluate('probe.data[1]')==0
                page.evaluate('probe.e.passive_scope(0)');page.mouse.move(31,25)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':30,'y':25,'id':4}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_function('probe.data[1]===1')
            # Independent host frames isolate animation invalidation from pointer redraws.
            for named in [0,1]:
                page.evaluate('probe.reset(0)')
                page.evaluate('(n)=>{probe.e.passive_scope(2);probe.e.passive_loading(1,n);probe.step()}',named)
                hidden=page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')
                assert page.evaluate('probe.e.passive_redraw()')==0
                assert page.evaluate('probe.e.passive_unchanged()')==1
                page.evaluate('probe.e.passive_scope(1);probe.step()')
                assert page.evaluate('probe.e.passive_redraw()')==1
                assert page.evaluate('probe.e.passive_unchanged()')==1
                assert page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')==hidden
                page.evaluate('probe.e.passive_scope(0);probe.step()')
                assert page.evaluate('probe.e.passive_redraw()')==1
            assert not errors,errors
            browser.close()
        print('PASS: actual hover/open tooltip, passive state/callback/pixels, mouse/CDP touch/keyboard normal reuse')
    finally:server.shutdown()
if __name__=='__main__':main()
