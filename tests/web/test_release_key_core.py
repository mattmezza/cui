# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual Chromium keyboard events routed into a Core-only WASI probe."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><button id="focus" type="button">Focus host</button><script>
(async()=>{
 const module=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer());
 const imports={};for(const i of WebAssembly.Module.imports(module)){
  (imports[i.module]??={})[i.name]=i.name==='proc_exit'?()=>{throw Error('WASI exit')}:()=>0;
 }
 const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();
 if(e.probe_init()||e.probe_frame())throw Error('init');
 window.probe={e,signals:[]};
 document.getElementById('focus').addEventListener('focus',()=>e.probe_focus(10));
 for(const type of ['keydown','keyup'])document.addEventListener(type,ev=>{
  const key=ev.key==='Enter'?2:ev.key===' '?3:ev.key==='Tab'?1:ev.key==='Escape'?4:0;
  if(key===1||key===2||key===3||key===4)ev.preventDefault();
  const mods=(ev.shiftKey?1:0)|(ev.ctrlKey?2:0)|(ev.altKey?4:0)|(ev.metaKey?8:0);
  if(e.probe_event(type==='keydown',key,mods,ev.repeat)||e.probe_frame())throw Error('key route');
  probe.signals.push({type,key,mods,repeat:ev.repeat,count:e.probe_stat(0)});
 });
})().catch(error=>{window.probeError=String(error);throw error;});
</script>'''

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'build/release-key-core-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    exports = ['probe_init', 'probe_event', 'probe_focus', 'probe_option', 'probe_frame', 'probe_stat']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1',
                    '-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),
                    '-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/release_key_core_probe.c'),
                    str(ROOT/'src/core/cui.c'),'-lm','-Wl,-z,stack-size=8388608',
                    *[f'-Wl,--export={name}' for name in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(out)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=args.chromium)
            page = browser.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}')
            page.wait_for_function('window.probe?.e.probe_stat(1)===1')
            page.locator('#focus').focus()
            assert page.evaluate('probe.e.probe_stat(2)') == 10
            def count(): return page.evaluate('probe.e.probe_stat(0)')
            page.keyboard.down('Enter'); assert count() == 0
            page.keyboard.up('Enter'); assert count() == 1
            page.keyboard.down('Enter'); assert count() == 1
            page.keyboard.up('Space'); assert count() == 2
            page.keyboard.down('Shift'); page.keyboard.down('Enter'); assert count() == 2
            page.keyboard.up('Enter'); page.keyboard.up('Shift'); assert count() == 2
            page.keyboard.down('Space'); assert count() == 2
            page.evaluate('probe.e.probe_focus(0);probe.e.probe_focus(10)')
            page.keyboard.up('Space'); assert count() == 2
            page.evaluate('probe.e.probe_option(0);probe.e.probe_frame()')
            page.keyboard.down('Enter'); assert count() == 3
            page.keyboard.up('Enter'); assert count() == 3
            signals = page.evaluate('probe.signals')
            assert any(s['type']=='keydown' and s['key']==3 for s in signals), signals
            assert any(s['type']=='keyup' and s['key']==3 for s in signals), signals
            assert not errors, errors
            browser.close()
    finally:
        server.shutdown()
    print('Core release key actual Chromium keyboard events PASS')
if __name__ == '__main__': main()
