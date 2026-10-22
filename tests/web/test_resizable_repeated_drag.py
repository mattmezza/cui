#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Repeated actual-WASM divider drags, release/regrab and responsive constraints."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_resizable_drag import drag, panes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for touch, dpr in ((False, 1), (False, 2), (True, 2)):
                context = browser.new_context(viewport={'width': 390 if touch else 1280, 'height': 1000}, has_touch=touch, device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page = context.new_page(); a.errors = []; a.active_touch = False
                a.page.on('pageerror', lambda error: a.errors.append(str(error)))
                a.session = context.new_cdp_session(a.page)
                a.page.goto(f'http://127.0.0.1:{server.server_port}/#Resizable')
                a.page.wait_for_function('window.cuiGallery?.renderedComponent === "Resizable"')
                for vertical in (False, True):
                    a.configure('resizable-vertical', vertical)
                    field = 4 if vertical else 3
                    position = 2 if vertical else 1
                    for delta in (18, -18, 22, -22, 16, -16):
                        before = panes(a)
                        drag(a, touch, vertical, delta)
                        after = panes(a)
                        actual = after[0]['f'][field] - before[0]['f'][field]
                        assert abs(actual - delta) < 2, (touch, dpr, vertical, delta, actual)
                        assert abs(after[1]['f'][field] - before[1]['f'][field] + actual) < 1
                        assert abs(after[1]['f'][position] - before[1]['f'][position] - actual) < 1
                        for pane in after:
                            assert a.page.evaluate('''b=>{const e=cuiGallery.exports;
                                for(let i=0;i<e.gallery_count();i++)
                                    if(e.gallery_command(i,0)===0&&[1,2,3,4].every((f,k)=>Math.abs(e.gallery_command(i,f)-b[k])<.5))return true;
                                return false;}''', pane['f'][1:5]), 'painted pane must follow resized bounds'
                    # Pointer clamps must affect actual pane geometry, not only a counter.
                    drag(a, touch, vertical, -500)
                    assert abs(panes(a)[0]['f'][field] - 80) < 1
                    drag(a, touch, vertical, 500)
                    extent = sum(p['f'][field] for p in panes(a))
                    assert abs(panes(a)[0]['f'][field] - min(300, extent - 80)) < 1
                    handle = a.target('Resize panels', 40)
                    if touch:
                        a.tap(handle)
                    else:
                        point = a.point(handle); a.page.mouse.click(point['x'], point['y'])
                    a.page.keyboard.press('Home'); a.page.wait_for_timeout(100)
                    assert abs(panes(a)[0]['f'][field] - 80) < 1
                    a.page.keyboard.press('ArrowDown' if vertical else 'ArrowRight'); a.page.wait_for_timeout(100)
                    assert abs(panes(a)[0]['f'][field] - 90) < 1
                    a.page.keyboard.press('End'); a.page.wait_for_timeout(100)
                    assert abs(panes(a)[0]['f'][field] - min(300, extent - 80)) < 1
                    a.configure('resizable-disabled', True)
                    locked = [p['f'][1:5] for p in panes(a)]
                    drag(a, touch, vertical, -20)
                    assert [p['f'][1:5] for p in panes(a)] == locked
                    a.configure('resizable-disabled', False)
                a.configure('resizable-vertical', False)
                for width in (500, 390, 800):
                    a.page.set_viewport_size({'width': width, 'height': 1000}); a.page.wait_for_timeout(150)
                    current = panes(a)
                    canvas_width = a.page.locator('canvas').evaluate('e=>e.getBoundingClientRect().width')
                    assert all(p['f'][3] >= 79.5 for p in current)
                    assert current[1]['f'][1] + current[1]['f'][3] <= canvas_width + .5
                    # A responsive shrink may legitimately land on the minimum.
                    drag(a, touch, False, 24)
                    current = panes(a)
                    drag(a, touch, False, -16)
                    assert panes(a)[0]['f'][3] < current[0]['f'][3] - 12
                assert a.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not a.errors, a.errors
                print(f'PASS repeated release/regrab, constraints, keys and resize touch={touch} DPR={dpr}', flush=True)
                context.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()


if __name__ == '__main__':
    main()
