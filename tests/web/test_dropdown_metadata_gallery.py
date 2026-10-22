#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM Dropdown names, traversal and legacy eligibility on mouse/touch."""
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
            for mobile, dpr in ((False, 1), (False, 2), (True, 2)):
                context = browser.new_context(viewport={'width': 390 if mobile else 1280, 'height': 1000}, has_touch=mobile, device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page = context.new_page(); a.errors = []; a.active_touch = False
                a.page.on('pageerror', lambda error: a.errors.append(str(error)))
                a.session = context.new_cdp_session(a.page)
                a.page.goto(f'http://127.0.0.1:{server.server_port}/#DropdownButton')
                a.page.wait_for_function('window.cuiGallery?.renderedComponent === "DropdownButton"')
                def activate(name):
                    target = a.target(name, 1)
                    if mobile:
                        a.tap(target)
                    else:
                        point = a.point(target); a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(80)
                def focused(name):
                    assert a.target(name, 1)['f'][7] == 1, a.snapshot()
                def key(value):
                    a.page.keyboard.press(value); a.page.wait_for_timeout(80)
                def outside():
                    assert not a.page.locator('canvas').evaluate('e=>e===document.activeElement')
                assert a.target('Export workspace', 1)
                activate('Export workspace')
                key('Tab'); focused('Export options')
                key('Shift+Tab'); focused('Export workspace')
                key('Shift+Tab'); outside()
                a.page.locator('canvas').focus(); key('Tab'); focused('Export workspace')
                key('Tab'); focused('Export options'); key('Tab'); outside()
                before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
                a.configure('dropdown-names', '1')
                assert a.target('Exporter le projet', 1) and a.target('Autres exports', 1)
                assert 'Export workspace' in a.texts(), 'visible label must stay unchanged'
                assert a.page.locator('#accessible button', has_text='Exporter le projet').count() == 1
                a.configure('dropdown-order', '2')
                assert a.target('Exporter le projet', 1)['f'][9] == 2
                assert a.target('Autres exports', 1)['f'][9] == -2
                a.configure('dropdown-toggle', '2')
                assert a.target('Exporter le projet', 1)['f'][6:9:2] == [1, 1]
                assert a.target('Autres exports', 1)['f'][6] == 0
                assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before
                compile_usage(a.page.locator('#snippet').inner_text())
                activate('Exporter le projet')
                assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before + 1
                # Actual canvas keyboard events use the CUI traversal loop.
                a.page.keyboard.press('Shift+Tab'); a.page.wait_for_timeout(80); focused('Autres exports')
                key('Shift+Tab'); outside()
                a.page.locator('canvas').focus(); key('Tab'); focused('Autres exports')
                key('Tab'); focused('Exporter le projet'); key('Tab'); outside()
                a.configure('dropdown-skip', '1')
                activate('Autres exports')
                a.page.keyboard.press('Escape'); a.page.wait_for_timeout(80)
                activate('Exporter le projet')
                before += 1
                a.page.locator('canvas').focus()
                a.page.keyboard.press('Shift+Tab'); a.page.wait_for_timeout(80); focused('Autres exports')
                key('Tab'); outside()
                a.page.locator('canvas').focus(); key('Shift+Tab'); focused('Autres exports')
                key('Shift+Tab'); outside()
                activate('Exporter le projet')  # skip-tab still allows pointer activation
                assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before + 2
                a.configure('dropdown-action', '2')
                assert a.target('Exporter le projet', 1)['f'][5] == 1
                assert a.target('Exporter le projet', 1)['f'][14] == 1
                activate('Exporter le projet')
                assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before + 2
                a.configure('disabled', True)
                assert a.target('Autres exports', 1)['f'][5] == 1
                a.configure('disabled', False); a.configure('dropdown-action', '0')
                a.configure('dropdown-skip', '2')
                activate('Exporter le projet')
                key('Tab'); outside()
                a.page.locator('canvas').focus(); key('Shift+Tab'); focused('Exporter le projet')
                key('Shift+Tab'); outside()
                a.configure('dropdown-halves', '1')
                assert not any(s['name'] == 'Exporter le projet' for s in a.snapshot())
                assert a.target('Autres exports', 1)
                a.configure('dropdown-halves', '2')
                assert not any(s['name'] == 'Autres exports' for s in a.snapshot())
                assert a.target('Exporter le projet', 1)
                a.configure('dropdown-halves', '0'); a.configure('dropdown-names', '2')
                assert len([s for s in a.snapshot() if s['f'][0] == 1 and s['name'] == '']) == 2
                compile_usage(a.page.locator('#snippet').inner_text())
                a.route('Button')
                for key in ('names', 'order', 'skip', 'toggle'):
                    assert a.page.locator('#dropdown-' + key).is_hidden()
                a.route('DropdownButton')
                for key in ('names', 'order', 'skip', 'toggle'):
                    assert a.page.locator('#dropdown-' + key).input_value() == '0'
                assert a.target('Export workspace', 1)['f'][6] == 0
                assert a.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not a.errors, a.errors
                print(f'PASS Dropdown metadata mobile={mobile} DPR={dpr}')
                context.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()


if __name__ == '__main__':
    main()
