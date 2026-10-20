#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Questionnaire shortcut OFF, letters/numbers and touch activation."""
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
                a.route('Questionnaire')

                def focus_choice():
                    a.page.locator('canvas').focus()
                    for _ in range(16):
                        if a.target('Build a native app', 4)['f'][7]:
                            return
                        a.page.keyboard.press('Tab')
                        a.page.wait_for_timeout(50)
                    raise AssertionError(('Tab did not focus choice',
                                          [n['name'] for n in a.snapshot() if n['f'][7]]))

                assert all(letter in a.texts() for letter in ('A', 'B', 'C'))
                a.page.locator('#questionnaire-shortcuts').uncheck()
                a.page.wait_for_timeout(80)
                assert not any(letter in a.texts() for letter in ('A', 'B', 'C'))
                # Disabling shortcuts also suppresses the custom adornment callback.
                a.page.locator('#questionnaire-custom').check()
                a.page.wait_for_timeout(80)
                assert a.texts().count('Custom content slot') == 3
                assert not any(letter in a.texts() for letter in ('A', 'B', 'C', '-'))
                focus_choice()
                a.page.keyboard.press('b')
                a.page.wait_for_timeout(220)
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(0)') == 0
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(4)') == 0
                if mobile:
                    # OFF is not a disabled control: real touch still selects it.
                    a.tap(a.target('Explore components', 4))
                else:
                    a.page.locator('#questionnaire-shortcuts').check()
                    a.page.wait_for_timeout(80)
                    assert all(letter in a.texts() for letter in ('A', 'B', 'C'))
                    focus_choice()
                    a.page.keyboard.press('b')
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===1')
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(9)') == 1
                # Model advancement happens during submission; wait for the next
                # composed question before reading reset-button geometry.
                for _ in range(60):
                    if any(n['name'] == 'Your note' for n in a.snapshot()):
                        break
                    a.page.wait_for_timeout(50)
                else:
                    raise AssertionError('Advanced question was not composed')
                # Reset, then prove number mode uses the same retained model.
                point = a.point(a.target('Start over', 1))
                # Give the immediate-mode host a frame for each physical phase;
                # an instantaneous click can otherwise coalesce under CI load.
                a.page.mouse.move(point['x'], point['y'])
                a.page.wait_for_timeout(80)
                a.page.mouse.down()
                a.page.wait_for_timeout(80)
                a.page.mouse.up()
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===0')
                a.page.locator('#questionnaire-shortcuts').check()
                a.page.locator('#questionnaire-numbers').check()
                a.page.wait_for_timeout(80)
                assert all(number in a.texts() for number in ('1', '2', '3'))
                focus_choice()
                a.page.keyboard.press('2')
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===1')
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(9)') == 1
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS: real Questionnaire OFF/custom hints/letters/numbers and selection; touch={mobile}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
