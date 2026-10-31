#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM proof of Spinner finite turns, reduced ease(0), rejection and sizing."""
import argparse
import math
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_spinner_gallery import IMAGE
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
            for width, dpr in ((1280, 1), (1280, 2), (390, 1), (390, 2)):
                context = browser.new_context(viewport={'width': width, 'height': 1000}, device_scale_factor=dpr, reduced_motion='reduce')
                page = context.new_page(); errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Spinner')
                page.wait_for_function('window.cuiGallery?.renderedComponent === "Spinner"')
                def configure(key, value):
                    page.locator('#spinner-' + key).select_option(str(value)); page.wait_for_timeout(65)
                def stat(field):
                    return page.evaluate('(f)=>cuiGallery.exports.gallery_spinner_contract(f)', field)
                def image():
                    return page.evaluate(IMAGE)
                def idle():
                    assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
                    def snapshot():
                        return page.evaluate("""() => ({
                            frame: cuiGallery.frame,
                            wake: cuiGallery.exports.gallery_next_wake(),
                            reduced: cuiGallery.renderedReducedMotion,
                            paints: cuiGallery.exports.gallery_spinner_contract(4),
                            easingCalls: cuiGallery.exports.gallery_spinner_contract(10),
                            scrollX, scrollY, now: performance.now()
                        })""")
                    before = snapshot()
                    shot = page.locator('canvas').screenshot()
                    after_screenshot = snapshot()
                    page.wait_for_timeout(100)
                    after_wait = snapshot()
                    assert after_wait['frame'] == before['frame'], {
                        'before': before, 'after_screenshot': after_screenshot,
                        'after_wait': after_wait, 'width': width, 'dpr': dpr,
                    }
                    assert page.locator('canvas').screenshot() == shot
                assert page.locator('#spinner-sizing').input_value() == '0'
                assert image()['rect'][2:4] == [16, 16]
                configure('icon', 2); configure('size', 64)
                zero = image()['mask']
                masks = []; shots = []
                for easing, expected in ((2, .25), (3, -.75), (4, 1.25), (5, .25)):
                    configure('easing', easing)
                    assert stat(0) == 1 and stat(1) == expected and stat(2) == 0
                    assert abs(stat(6) - 1.5707963267948966) < .00001
                    assert stat(11) == 0, 'reduced easing must receive raw zero'
                    masks.append(image()['mask']); shots.append(page.locator('canvas').screenshot())
                    idle()
                    if width == 1280 and dpr == 1:
                        compile_usage(page.locator('#snippet').inner_text())
                assert len({bytes(mask) for mask in masks}) == 1
                assert len(set(shots)) == 1
                assert masks[0] != zero, 'asymmetric SVG must actually rotate'
                # Independent field comparisons avoid padding-dependent snapshots.
                for easing in (6, 7, 8):
                    previous = [stat(f) for f in (1, 2, 3, 4, 8, 9)]
                    calls = stat(10); configure('easing', easing)
                    assert stat(0) == 0 and stat(7) == 1 and stat(10) > calls
                    assert [stat(f) for f in (1, 2, 3, 4, 8, 9)] == previous
                    assert image() is None, 'nonfinite easing must not paint'
                    idle()
                    if width == 1280 and dpr == 1:
                        compile_usage(page.locator('#snippet').inner_text())
                configure('easing', 2)
                assert stat(0) == 1 and image()['mask'] == masks[0]
                # Named sizes scale with explicit host rem for default/custom painters.
                for icon in (0, 2):
                    configure('icon', icon)
                    for sizing, ratio in ((1, .75), (2, .875), (3, 1), (4, 1.5)):
                        configure('sizing', sizing)
                        for rem in (0, 16, 20, 32):
                            configure('rem', rem)
                            expected = (rem or 16) * ratio
                            assert [stat(12),stat(13)] == [expected,expected]
                            assert all(expected<=value<=math.ceil(expected)+1 for value in image()['rect'][2:4])
                            assert any(image()['mask'][3::4])
                            if icon:
                                assert stat(5) == expected
                    for sizing in (0, 5):
                        configure('sizing', sizing)
                        for rem in (16, 20, 32):
                            configure('rem', rem)
                            assert [stat(12),stat(13)] == [64,64]
                            assert all(64<=value<=65 for value in image()['rect'][2:4])
                            if icon:
                                assert stat(5) == 64
                    if width == 1280 and dpr == 1:
                        compile_usage(page.locator('#snippet').inner_text())
                # Reject while moving too: do not schedule the retained old result.
                configure('easing', 5)
                page.emulate_media(reduced_motion='no-preference')
                page.wait_for_function('cuiGallery.renderedReducedMotion === false')
                assert stat(0) == 1 and stat(2) == 1
                configure('easing', 6)
                assert stat(0) == 0 and stat(7) == 1 and image() is None
                paints = stat(4); idle(); assert stat(4) == paints
                configure('easing', 5)
                assert stat(0) == 1 and stat(2) == 1
                page.emulate_media(reduced_motion='reduce')
                page.wait_for_function('cuiGallery.renderedReducedMotion === true')
                assert stat(1) == .25 and stat(11) == 0 and stat(2) == 0
                idle()
                # A preference change must be consumed by its first rendered frame,
                # rather than leaving Core dirty and racing the idle observation.
                if width == 390 and dpr == 2:
                    for _ in range(16):
                        page.emulate_media(reduced_motion='no-preference')
                        page.wait_for_function('cuiGallery.renderedReducedMotion === false')
                        page.emulate_media(reduced_motion='reduce')
                        page.wait_for_function('cuiGallery.renderedReducedMotion === true')
                        assert page.evaluate('cuiGallery.exports.gallery_redraw()') == 0
                        assert stat(1) == .25 and stat(11) == 0 and stat(2) == 0
                        idle()
                assert page.locator('#accessible [role=status],#accessible [aria-busy=true]').count() == 0
                page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
                page.wait_for_function('window.cuiGallery?.renderedComponent === "Button"')
                assert page.locator('#spinner-sizing').is_hidden() and page.locator('#spinner-rem').is_hidden()
                assert page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not errors, errors
                print(f'PASS Spinner finite/reduced/nonfinite/rem contracts width={width} DPR={dpr}', flush=True)
                context.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()


if __name__ == '__main__':
    main()
