#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real optional DateTime WASM popup; desktop and CDP touch, no direct value mutation."""
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
                    a.page.wait_for_timeout(100)

                def absent(name):
                    assert not any(s['name'] == name for s in a.snapshot()), name

                def value(date, time):
                    a.page.wait_for_function('([d,t])=>cuiGallery.exports.gallery_date_value()===d && cuiGallery.exports.gallery_control_stats(12)===t', arg=[date, time])

                def modal(expected):
                    assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == expected

                def snippet(seconds, twelve, clear, locale='EN_US'):
                    code = a.page.locator('#snippet').inner_text()
                    assert '#include "cui/cui_date_time_picker.h"' in code
                    assert ('CUI_DATE_TIME_SECOND' if seconds else 'CUI_DATE_TIME_MINUTE') in code
                    assert f'.twelve_hour = {str(twelve).lower()}' in code
                    assert f'.cleanable = {str(clear).lower()}' in code
                    assert f'CUI_DATE_LOCALE_{locale}' in code
                    a.page.context.grant_permissions(['clipboard-read', 'clipboard-write'])
                    a.page.locator('#copy-snippet').click()
                    a.page.wait_for_timeout(80)
                    assert a.page.evaluate('navigator.clipboard.readText()') == code
                    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                    '-I', str(root / 'include'), '-x', 'c', '-fsyntax-only', '-'],
                                   input=code, text=True, check=True, capture_output=True)

                assert 'cui_date_picker_ex(' in a.page.locator('#snippet').inner_text()
                absent('Include seconds'); absent('12-hour clock')
                click('Date + time', 2)
                value(20261012, 93000)
                absent('Date-only actions')
                snippet(False, False, False)
                click('Date and time')
                modal(1)
                absent('Second'); absent('AM/PM')
                a.page.keyboard.press('Escape')
                click('Include seconds', 2)
                click('12-hour clock', 2)
                click('Clear button and Delete', 2)
                snippet(True, True, True)
                click('Date and time')
                click('Minute', 6)
                a.page.keyboard.type('4')
                value(20261012, 90400)
                a.page.wait_for_timeout(180)  # Idle repaint preserves one-digit draft.
                a.page.keyboard.type('5')
                value(20261012, 94500)
                click('Second', 6)
                a.page.keyboard.type('5')
                value(20261012, 94505)
                a.page.wait_for_timeout(180)
                a.page.keyboard.type('9')
                value(20261012, 94559)
                click('AM/PM', 6)
                a.page.keyboard.type('p')
                value(20261012, 214559)
                click('10/13/2026')
                value(20261013, 214559); modal(1)
                click('10/13/2026')
                modal(0)
                click('Date and time'); click('Launch day')
                value(20261105, 214559); modal(0)
                click('Date and time'); click('Timed review')
                value(20261218, 231530); modal(0)
                click('Include seconds', 2)
                value(20261218, 231500)
                click('Include seconds', 2)
                value(20261218, 231500)  # Raising precision cannot restore dropped seconds.
                click('Clear date')
                value(0, 93000); absent('Clear date')
                click('Date and time'); click('Launch day')
                value(20261105, 93000)
                # Precision truncation and period controls remain opt-in.
                click('Include seconds', 2)
                click('12-hour clock', 2)
                click('Date and time'); absent('Second'); absent('AM/PM')
                a.page.keyboard.press('Escape')
                click('Date + time', 2)
                assert 'cui_date_picker_ex(' in a.page.locator('#snippet').inner_text()
                a.target('11/05/2026', 1); absent('Include seconds')
                click('Date + time', 2)
                value(20261105, 93000)
                a.configure('date-locale', 3)
                snippet(False, False, True, 'FR')
                click('Date and time'); click('06/11/2026')
                value(20261106, 93000); modal(1)
                click('06/11/2026'); modal(0)
                click('Date and time')
                a.configure('disabled', True)
                modal(0); absent('Minute'); absent('Launch day')
                assert a.target('Date and time', 1)['f'][5] == 1
                click('Date and time'); value(20261106, 93000); modal(0)
                a.configure('disabled', False)
                click('Date and time'); click('Minute', 6)
                a.page.keyboard.press('Delete')
                value(0, 93000); modal(0); absent('Clear date')
                # Date-only actions retain their opt-in selection when returning from time mode.
                click('Date + time', 2)
                click('Date-only actions', 2)
                click('Date'); click('Review day')
                value(20261218, 93000)
                click('Date + time', 2)
                value(20261218, 93000)
                click('Date and time'); click('Launch day')
                value(20261105, 93000)
                click('Date + time', 2)
                assert 'cui_date_picker_actions(' in a.page.locator('#snippet').inner_text()
                a.target('05/11/2026', 1); absent('Include seconds')
                click('Date + time', 2); click('Date and time'); modal(1)
                a.route('Button'); modal(0)
                a.route('DatePicker'); modal(0)
                value(20261105, 93000)
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS: DateTime real segments/drafts/day confirmation/presets/clear/locale/disabled/state transfer/copied C11 snippet; touch={touch}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
