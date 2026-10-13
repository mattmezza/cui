# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM copied Button click payload and callback policy."""
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
HTML = HTML.replace(" function queue(", " function modifiers(e){return Number(e.shiftKey)|Number(e.ctrlKey)*2|Number(e.altKey)*4|Number(e.metaKey)*8;}\n function queue(")
HTML = HTML.replace("dy))throw Error('queue')", "dy,modifiers(ev),ev.detail))throw Error('queue')")
HTML = HTML.replace("e.probe_key(key,0,Number(ev.repeat))", "e.probe_key(key,0,Number(ev.repeat),modifiers(ev))")
HTML = HTML.replace("e.probe_key(key,1,0)", "e.probe_key(key,1,0,modifiers(ev))")
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-click-event-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','click_stat']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_click_event_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
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
            def reset(mode):
                page.evaluate('(m)=>probe.reset(m)',mode)
            def point():
                b=page.evaluate('Array.from({length:4},(_,i)=>probe.e.probe_rect(20,i))')
                return b[0]+b[2]/2+10,b[1]+b[3]/2+10
            def payload(index):
                return page.evaluate('(i)=>Array.from({length:12},(_,f)=>probe.e.click_stat(i,f+2))',index)
            reset(0);x,y=point()
            page.keyboard.down('Shift');page.mouse.click(x,y);page.keyboard.up('Shift')
            page.wait_for_function('probe.e.click_stat(0,0)===1')
            first=payload(0)
            assert first[0:2]==[0,20] and first[4]==1 and first[5]==1,first
            assert first[9]==1 and abs(first[10]-(x-10))<.1 and abs(first[11]-(y-10))<.1,first
            cdp=page.context.new_cdp_session(page)
            cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':71}]})
            cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
            page.wait_for_function('probe.e.click_stat(0,0)===2')
            touch=payload(1)
            assert touch[0]==0 and touch[6]==1 and touch[7]>0,touch
            page.keyboard.press('Enter');page.keyboard.press('Space')
            page.wait_for_function('probe.e.click_stat(0,0)===4')
            assert payload(2)[0]==1 and payload(2)[8]==2 and payload(2)[9]==0
            assert payload(3)[0]==1 and payload(3)[8]==3
            assert payload(0)==first,'copied payload must survive later frames/input'
            assert page.evaluate('probe.e.click_stat(0,1)')==0
            for mode in [1,2,5,6]:
                reset(mode);x,y=point();page.mouse.click(x,y)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':72}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_timeout(80)
                assert page.evaluate('probe.e.click_stat(0,0)')==0
                assert page.evaluate('probe.data[1]')==0
            reset(0);x,y=point()
            cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':x,'y':y,'id':73}]})
            cdp.send('Input.dispatchTouchEvent',{'type':'touchCancel','touchPoints':[]})
            page.wait_for_timeout(80)
            assert page.evaluate('probe.e.click_stat(0,0)')==0
            page.mouse.click(x,y);page.wait_for_function('probe.e.click_stat(0,0)===1')
            for mode in [3,4]:
                reset(mode);x,y=point();page.mouse.click(x,y)
                if mode==3:
                    page.wait_for_function('probe.e.click_stat(0,0)===1')
                    assert payload(0)[1]==20 and page.evaluate('probe.e.click_stat(0,1)')==0
                else:
                    page.wait_for_function('probe.data[0]===1')
                    assert page.evaluate('probe.e.click_stat(0,0)')==0
                    assert page.evaluate('probe.data[1]')==0
            assert not errors,errors
            browser.close()
        print('PASS: copied pointer/key payload, modifiers, touch/cancel, disabled/passive and Group policy')
    finally:server.shutdown()
if __name__=='__main__':main()
