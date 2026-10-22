#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Nested real-WASM Resizable capture, geometry, keyboard and teardown."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_questionnaire_progress_gallery import compile_usage


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
                def handle(inner):
                    return a.target('Resize panels', 40, lambda s: (s['f'][3] > s['f'][4]) == inner)
                def panel(name):
                    return a.target(name)['f'][1:5]
                def geometry():
                    return [panel(name) for name in ('Explorer panel', 'Workspace panel', 'Code panel', 'Console panel')]
                def begin(inner):
                    start = a.point(handle(inner))
                    if touch:
                        a.dispatch('touchStart', [start])
                    else:
                        a.page.mouse.move(start['x'], start['y']); a.page.mouse.down()
                    return start
                def end():
                    if touch:
                        a.dispatch('touchEnd', [])
                    else:
                        a.page.mouse.up()
                    a.page.wait_for_timeout(80)
                def drag(inner, delta):
                    start = begin(inner)
                    for step in range(1, 7):
                        point = {**start, 'x': start['x'] + (0 if inner else delta * step / 6),
                                 'y': start['y'] + (delta * step / 6 if inner else 0)}
                        if touch:
                            a.dispatch('touchMove', [point])
                        else:
                            a.page.mouse.move(point['x'], point['y'])
                    end()
                def focus(inner):
                    target = handle(inner)
                    if touch:
                        a.tap(target)
                    else:
                        point = a.point(target); a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(80)
                assert len([s for s in a.snapshot() if s['f'][0] == 40]) == 1
                compile_usage(a.page.locator('#snippet').inner_text())
                a.configure('resizable-nested', True)
                assert a.page.locator('#resizable-vertical-control').is_hidden()
                assert len([s for s in a.snapshot() if s['f'][0] == 40]) == 2
                compile_usage(a.page.locator('#snippet').inner_text())
                for inner in (False, True):
                    for delta in (18, -18, 20, -20):
                        before = geometry(); drag(inner, delta); after = geometry()
                        if inner:
                            assert after[:2] == before[:2], 'inner capture resized outer group'
                            assert abs(after[2][3] - before[2][3] - delta) < 1
                            assert abs(after[3][3] - before[3][3] + delta) < 1
                            assert abs(after[3][1] - before[3][1] - delta) < 1
                        else:
                            assert abs(after[0][2] - before[0][2] - delta) < 1
                            assert abs(after[1][2] - before[1][2] + delta) < 1
                            assert after[2][3] == before[2][3] and after[3][3] == before[3][3]
                            assert abs(after[2][2] - after[1][2]) < 1
                            assert abs(after[3][2] - after[1][2]) < 1
                        for bounds in after:
                            assert bounds[2] > 0 and bounds[3] > 0
                            assert a.page.evaluate('''b=>{const e=cuiGallery.exports;
                                for(let i=0;i<e.gallery_count();i++)
                                    if(e.gallery_command(i,0)===0&&[1,2,3,4].every((f,k)=>Math.abs(e.gallery_command(i,f)-b[k])<.5))return true;
                                return false;}''', bounds), 'paint must follow nested pane geometry'
                focus(True)
                before = geometry()
                a.page.keyboard.press('ArrowUp'); a.page.wait_for_timeout(100)
                assert geometry()[:2] == before[:2]
                assert abs(geometry()[2][3] - before[2][3] + 10) < 1
                a.page.keyboard.press('Home'); a.page.wait_for_timeout(80)
                assert abs(panel('Code panel')[3] - 100) < 1
                a.page.keyboard.press('End'); a.page.wait_for_timeout(80)
                assert abs(panel('Code panel')[3] - 240) < 1
                focus(False); before = geometry()
                a.page.keyboard.press('ArrowLeft'); a.page.wait_for_timeout(100)
                assert abs(geometry()[0][2] - before[0][2] + 10) < 1
                assert geometry()[2][3] == before[2][3]
                before = geometry(); drag(True, -500)
                assert geometry()[:2] == before[:2] and abs(panel('Code panel')[3] - 100) < 1
                drag(True, 500)
                assert abs(panel('Code panel')[3] - 240) < 1
                before = geometry(); drag(False, 500)
                total = before[0][2] + before[1][2]
                assert abs(panel('Explorer panel')[2] - min(300, total - 80)) < 1
                assert panel('Code panel')[3] == before[2][3]
                drag(False, -500)
                assert abs(panel('Explorer panel')[2] - 80) < 1
                for control in ('resizable-disabled', 'disabled'):
                    a.configure(control, True); locked = geometry()
                    assert handle(True)['f'][5] and handle(False)['f'][5]
                    drag(True, -20); drag(False, 20)
                    assert geometry() == locked
                    a.configure(control, False)
                # A route transition during capture must leave no retained drag.
                begin(True); a.route('Button'); end()
                assert a.page.locator('#resizable-nested-control').is_hidden()
                a.route('Resizable')
                assert not a.page.locator('#resizable-nested').is_checked()
                assert len([s for s in a.snapshot() if s['f'][0] == 40]) == 1
                a.configure('resizable-nested', True)
                before = geometry(); drag(True, 18)
                assert geometry()[:2] == before[:2]
                assert abs(geometry()[2][3] - before[2][3] - 18) < 1
                assert a.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not a.errors, a.errors
                print(f'PASS nested capture/constraints/keyboard/teardown touch={touch} DPR={dpr}', flush=True)
                context.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()


if __name__ == '__main__':
    main()
