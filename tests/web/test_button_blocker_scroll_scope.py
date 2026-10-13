# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real wheel/CDP touch over an inert sibling overlay above a Scrollable."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
from test_button_activation_blocker import HTML

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT/'build/blocker-scroll-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args()
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    source = args.source_root.resolve()
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((source/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    exports = ['probe_init', 'probe_event', 'probe_key', 'probe_frame', 'probe_page_available', 'probe_page_end', 'probe_stat', 'probe_rect', 'probe_count', 'probe_command', 'probe_text', 'probe_text_length', 'probe_bitmap', 'probe_scope']
    subprocess.run(['zig', 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(source/'include'), '-I', str(source/'third_party/clay'), str(ROOT/'tests/web/button_blocker_scroll_probe.c'), *[str(source/s) for s in sources if s.startswith('src/')], '-lm', '-Wl,-z,stack-size=8388608', *[f'-Wl,--export={e}' for e in exports], '-o', str(out/'probe.wasm')], check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(out)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, executable_path=args.chromium)
            page = browser.new_page(viewport={'width': 480, 'height': 600}, has_touch=True)
            page.goto(f'http://127.0.0.1:{server.server_port}')
            page.wait_for_function('window.probe?.frames>=3')
            session = page.context.new_cdp_session(page)
            def touch(kind, points):
                session.send('Input.dispatchTouchEvent', {'type': kind, 'touchPoints': points})
            for mode in [1, 2]:
                for contact in [False, True]:
                    page.evaluate('(mode)=>probe.reset(mode)', mode)
                    point = {'x': 70, 'y': 66, 'id': 1}
                    if contact:
                        touch('touchStart', [point])
                        page.wait_for_function('probe.events.at(-1)===1')
                        assert page.evaluate('[probe.e.probe_scope(0),probe.e.probe_scope(1)]') == [0, 1]
                        touch('touchMove', [{**point, 'y': 46}])
                    else:
                        page.mouse.move(point['x'], point['y']); page.mouse.wheel(0, 20)
                    page.wait_for_function('probe.data[3]===20')
                    assert page.evaluate('[probe.e.probe_scope(0),probe.e.probe_scope(1)]') == [1, 0], ('scroll scope blocked', mode, contact)
                    assert page.evaluate('probe.data[1]===0&&probe.fallbacks===0&&scrollY===0')
                    if contact:
                        touch('touchEnd', [])
                        page.wait_for_function('probe.events.at(-1)===2')
                        assert page.evaluate('probe.data[1]===0&&probe.data[3]===20')
            page.screenshot(path=str(out/'overlap-scroll.png'))
            browser.close()
        print('Sibling inert overlay: wheel/touch scroll scopes and activation separation PASS')
    finally:
        server.shutdown()

if __name__ == '__main__':
    main()
