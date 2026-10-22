#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM date-only actions with desktop pointer/keyboard and CDP touch."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True)
            for touch in (False, True):
                a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                if touch:
                    a.page.set_viewport_size({'width': 390, 'height': 844})
                a.route('DatePicker')
                a.configure('date-locale', 0)

                def click(name, role=1):
                    target = a.target(name, role)
                    if touch:
                        a.tap(target)
                    else:
                        point = a.point(target)
                        a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(90)

                def value(expected):
                    a.page.wait_for_function('(n)=>cuiGallery.exports.gallery_date_value()===n', arg=expected)

                def absent(name):
                    assert not any(s['name'] == name for s in a.snapshot()), name

                def copy_compile(clear):
                    code = a.page.locator('#snippet').inner_text()
                    assert '#include "cui/cui_date_picker_actions.h"' in code
                    assert f'.cleanable = {str(clear).lower()}' in code
                    assert ('CUI_DATE_PICKER_DELETE_SCOPE' if clear else 'CUI_DATE_PICKER_DELETE_OFF') in code
                    a.page.context.grant_permissions(['clipboard-read', 'clipboard-write'])
                    a.page.locator('#copy-snippet').click()
                    a.page.wait_for_timeout(80)
                    assert a.page.evaluate('navigator.clipboard.readText()') == code
                    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                    '-I', str(root / 'include'), '-x', 'c', '-fsyntax-only', '-'],
                                   input=code, text=True, check=True, capture_output=True)

                # Default story still uses the legacy API, without action targets.
                assert 'cui_date_picker_ex(' in a.page.locator('#snippet').inner_text()
                absent('Clear button and Delete')
                absent('Clear date')
                click('10/12/2026')
                absent('Launch day')
                a.page.keyboard.press('Escape')
                value(20261012)
                click('Date-only actions', 2)
                value(20261012)
                copy_compile(False)
                absent('Clear date')
                click('10/12/2026')
                click('Launch day')
                value(20261105)
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                a.page.keyboard.press('Delete')
                value(20261105)  # Delete policy remains OFF until explicitly enabled.
                click('Clear button and Delete', 2)
                copy_compile(True)
                assert a.target('Clear date', 1)['f'][5] == 0
                click('Clear date')
                value(0)
                absent('Clear date')
                click('Date')
                click('Review day')
                value(20261218)
                # Preset focuses its trigger; real browser Delete clears once.
                a.page.keyboard.press('Delete')
                value(0)
                click('Date')
                click('Launch day')
                value(20261105)
                # Scope Delete also works from a real Calendar header in its popup.
                click('11/05/2026')
                click('Choose month')
                a.page.keyboard.press('Delete')
                value(0)
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                click('Date')
                click('Launch day')
                value(20261105)
                click('11/05/2026')
                # Host disabled state closes the popup and gates every action.
                a.configure('disabled', True)
                absent('Launch day')
                assert a.target('Clear date', 1)['f'][5] == 1
                assert a.target('11/05/2026', 1)['f'][5] == 1
                click('Clear date')
                click('11/05/2026')
                a.page.keyboard.press('Delete')
                value(20261105)
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                a.configure('disabled', False)
                click('Clear button and Delete', 2)
                absent('Clear date')
                click('Date-only actions', 2)
                value(20261105)
                assert 'cui_date_picker_ex(' in a.page.locator('#snippet').inner_text()
                absent('Clear button and Delete')
                click('11/05/2026')
                absent('Launch day')
                a.page.keyboard.press('Escape')
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS: DatePicker opt-in presets, optional clear/Delete, disabled, legacy absence and compiled copied snippet; touch={touch}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
