#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Split-button paint policies and shadow gates through real WASM and input."""
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
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for mobile, dpr in ((False, 1), (False, 2), (True, 2)):
                context = browser.new_context(viewport={'width':390 if mobile else 1280,'height':1000},has_touch=mobile,device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page = context.new_page(); a.errors = []; a.active_touch = False
                a.page.on('pageerror', lambda error:a.errors.append(str(error)))
                a.session = context.new_cdp_session(a.page)
                a.page.goto(f'http://127.0.0.1:{server.server_port}/#DropdownButton')
                a.page.wait_for_function('window.cuiGallery?.renderedComponent === "DropdownButton"')
                def activate(name, role=1):
                    target = a.target(name, role)
                    if mobile:
                        a.tap(target)
                    else:
                        point = a.point(target)
                        a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(80)
                def shadows():
                    return a.page.evaluate('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===7).length;}')
                assert a.page.locator('#dropdown-paint').input_value() == '1'
                assert a.page.locator('#dropdown-shadow').is_disabled()
                for policy in ('0', '1'):
                    a.configure('dropdown-paint', policy)
                    compile_usage(a.page.locator('#snippet').inner_text())
                    before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
                    activate('Export workspace')
                    assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1
                    activate('Export options')
                    assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
                    activate('Priority', 16)
                    assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                    assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1
                a.configure('dropdown-variant', '11')
                assert not a.page.locator('#dropdown-shadow').is_disabled()
                assert shadows() == 0
                a.configure('dropdown-shadow', True)
                assert shadows() == 2
                compile_usage(a.page.locator('#snippet').inner_text())
                a.configure('dropdown-action', '2')
                before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
                activate('Export workspace')
                assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before
                assert shadows() == 2
                a.configure('disabled', True)
                assert shadows() == 0
                activate('Export options')
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                a.configure('disabled', False)
                a.configure('dropdown-action', '0')
                a.configure('dropdown-paint', '0')
                assert a.page.locator('#dropdown-shadow').is_disabled()
                assert shadows() == 0
                a.route('Button')
                assert a.page.locator('#dropdown-paint').is_hidden()
                assert a.page.locator('#dropdown-shadow-control').is_hidden()
                a.route('DropdownButton')
                assert a.page.locator('#dropdown-paint').input_value() == '1'
                assert not a.page.locator('#dropdown-shadow').is_checked()
                assert a.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not a.errors, a.errors
                print(f'PASS DropdownButton policies, shadow gates and input mobile={mobile} DPR={dpr}')
                context.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()


if __name__ == '__main__':
    main()
