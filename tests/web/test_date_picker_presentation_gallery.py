#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual DatePicker month menu, neighboring dates and popup dismissal."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


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
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True)
            for mobile in (False, True):
                a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                if mobile:
                    a.page.set_viewport_size({'width': 390, 'height': 844})
                a.route('DatePicker')
                a.configure('date-locale', 0)

                def click(name):
                    target = a.target(name, 1)
                    if mobile:
                        a.tap(target)
                    else:
                        point = a.point(target)
                        a.page.mouse.click(point['x'], point['y'])
                        a.page.wait_for_timeout(80)

                click('10/12/2026')
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
                click('Choose month')
                click('February')
                assert a.page.evaluate('cuiGallery.exports.gallery_date_value()') == 20261012
                # Trailing dates are real targets, including the next month's first day.
                click('03/01/2026')
                a.page.wait_for_function('cuiGallery.exports.gallery_date_value()===20260301')
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                assert '03/01/2026' in a.texts()
                click('03/01/2026')
                click('Choose month')
                a.page.keyboard.press('Escape')
                a.page.wait_for_timeout(80)
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
                a.page.keyboard.press('Escape')
                a.page.wait_for_timeout(80)
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                a.page.locator('#disabled').check()
                click('03/01/2026')
                assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS: DatePicker month menu, adjacent-date commit, Escape and disabled trigger; touch={mobile}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
