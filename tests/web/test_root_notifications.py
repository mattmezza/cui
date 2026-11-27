#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM opt-in Root notifications: responsive geometry and real touch at two widths."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading

from playwright.sync_api import sync_playwright

from test_touch_components import Audit


def assert_notice_geometry(audit):
    canvas = audit.page.locator('canvas').bounding_box()
    close = audit.target('Close', 1)
    action = audit.target('Review', 1)
    for target in (close, action):
        x, y, width, height = target['f'][1:5]
        assert width > 0 and height > 0, target
        assert x >= 0 and y >= 0, target
        assert x + width <= canvas['width'] + 1, (target, canvas)
        assert y + height <= canvas['height'] + 1, (target, canvas)
    x, y, width, height = close['f'][1:5]
    action_x, action_y, action_width, action_height = action['f'][1:5]
    assert (
        x + width <= action_x or action_x + action_width <= x
        or y + height <= action_y or action_y + action_height <= y
    ), (close, action)
    text = ' '.join(' '.join(audit.texts()).split())
    assert 'with your team.' in text, text
    return close, action


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass

    handler = partial(Handler, directory=str(args.directory.resolve()))
    server = ThreadingHTTPServer(('127.0.0.1', 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True, executable_path=args.chromium)
            for width in (1280, 390):
                audit = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                page = audit.page
                page.set_viewport_size({'width': width, 'height': 900})
                audit.route('Root View')
                assert page.evaluate('cuiGallery.exports.gallery_root_stats(0)') == 1
                assert all(
                    page.evaluate('(f)=>cuiGallery.exports.gallery_root_stats(f)', f) > 0
                    for f in (1, 2, 3, 4)
                )
                # CDP touch never introduces mouse hover: Close must already exist.
                audit.tap(audit.target('Notify workspace', 1))
                page.wait_for_function('cuiGallery.exports.gallery_root_stats(7)===1')
                assert_notice_geometry(audit)
                # The host must consume returned deadlines until entrance motion settles.
                page.wait_for_function('cuiGallery.exports.gallery_next_wake()===0')
                _, action = assert_notice_geometry(audit)
                audit.tap(action)
                page.wait_for_function('cuiGallery.exports.gallery_root_stats(10)===1')
                # A persistent notice remains available after Review.
                close, _ = assert_notice_geometry(audit)
                audit.tap(close)
                page.wait_for_function('cuiGallery.exports.gallery_root_stats(7)===0')
                assert 'Workspace update reviewed' in audit.texts()
                assert 'cui_root_render_with_options' in page.locator('#snippet').text_content()
                assert not audit.errors, audit.errors
                page.close()
                print(f'PASS: Root responsive notice Close before hover, action geometry and touch at {width}px', flush=True)
            browser.close()
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
