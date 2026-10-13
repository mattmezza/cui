#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM bar paint, hidden wheel/touch and bounded auto-hide checks."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


def bars(a):
    return [s for s in a.snapshot() if s['name'] in ('Vertical scroll', 'Horizontal scroll', 'Tab scrollbar')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = playwright.chromium.launch(**options)
            a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
            a.configure('scrollbar-policy', 1)
            a.configure('scrollbar-width', 16)
            a.configure('scrollbar-radius', 6)
            a.configure('scrollbar-track', False)
            for family in ('Scrollable', 'VirtualList', 'DataTable', 'MessageScroller', 'Sidebar', 'Attachment'):
                a.route(family)
                a.page.wait_for_timeout(120)
                visible = bars(a)
                assert visible, f'{family}: missing visible overflow bar'
                assert any(abs(min(s['f'][3], s['f'][4])-16) < .1 for s in visible), f'{family}: width ignored'
                a.configure('scrollbar-policy', 2)
                assert not bars(a), f'{family}: hidden bar remains interactive'
                owners = [s for s in a.snapshot() if s['f'][17] and (s['f'][20] > 0 or s['f'][21] > 0)]
                assert owners, f'{family}: hiding lost scroll owner'
                a.configure('scrollbar-policy', 1)
                print(f'PASS: {family}: visible width + hidden semantics/owner', flush=True)
            for family in ('Textarea', 'Editor'):
                a.route(family)
                if family == 'Textarea':
                    a.configure('textarea-rows', 3)
                target = a.target('Project notes' if family == 'Textarea' else 'C code editor', 6)
                a.tap(target)
                a.page.keyboard.press('Control+A')
                a.page.keyboard.insert_text('\n'.join(f'Actual editable fixture line {i}' for i in range(20)))
                a.page.wait_for_timeout(120)
                assert bars(a), f'{family}: no overlay after actual text input'
                a.configure('scrollbar-policy', 2)
                assert not bars(a), f'{family}: hidden editor bar remains'
                a.configure('scrollbar-policy', 1)
                print(f'PASS: {family}: real editable overflow + hidden overlay', flush=True)
            a.route('Tabs')
            for axis in (1, 2):
                a.configure('tabs-mode', axis)
                # The tab viewport bounds are measured from the preceding frame.
                a.configure('tabs-mode', axis)
                assert bars(a), f'Tabs: missing axis {axis} bar: ' + str(a.snapshot())
                a.configure('scrollbar-policy', 2)
                assert not bars(a), 'Tabs: hidden bar remains'
                a.configure('scrollbar-policy', 1)
            a.route('Carousel')
            a.configure('carousel-loop', False)
            assert bars(a), 'Finite carousel: missing bar'
            a.configure('carousel-loop', True)
            assert not bars(a), 'Infinite carousel: fabricated finite bar'
            a.route('TextView')
            a.configure('text-view-scroll', True)
            assert bars(a), 'TextView: missing body bar'
            a.configure('scrollbar-policy', 2)
            assert not bars(a), 'TextView: hidden bar remains'
            a.configure('scrollbar-policy', 1)
            for theme in (0, 1):
                a.configure('theme', theme)
                a.route('Scrollable')
                assert bars(a), 'Light/dark policy lost bars'
            a.route('Scrollable')
            a.configure('scrollbar-policy', 2)
            owner = next(s for s in a.snapshot() if s['f'][17] and s['f'][21] > 0)
            point = a.point(owner, .3, .3)
            before = a.value(owner, 19)
            a.page.mouse.move(point['x'], point['y'])
            a.page.mouse.wheel(0, 70)
            a.page.wait_for_timeout(100)
            assert a.value(owner, 19) > before, 'Hidden bars prevented wheel scrolling'
            before = a.value(owner, 19)
            a.dispatch('touchStart', [point])
            a.dispatch('touchMove', [{**point, 'y': point['y']-35}])
            a.dispatch('touchEnd', [])
            assert a.value(owner, 19) > before, 'Hidden bars prevented touch panning'
            a.configure('scrollbar-policy', 3)
            a.page.mouse.move(0, 0)
            a.page.wait_for_timeout(1300)
            assert not bars(a), 'Auto-hide retained interactive bars after idle'
            assert not a.page.evaluate('cuiGallery.exports.gallery_redraw()'), 'Auto-hide requests frames forever'
            a.configure('scrollbar-policy', 1)
            target = bars(a)[0]
            point = a.point(target)
            a.page.mouse.move(point['x'], point['y'])
            a.page.mouse.down()
            a.configure('scrollbar-policy', 3)
            a.page.wait_for_timeout(1300)
            assert bars(a), 'Captured bar disappeared'
            a.page.mouse.up()
            a.page.mouse.move(0, 0)
            a.page.wait_for_timeout(1300)
            assert not bars(a), 'Released bar never hid'
            assert not a.errors, a.errors
            browser.close()
    finally:
        server.shutdown()
    print('PASS: hidden wheel/touch and bounded idle/capture behavior')


if __name__ == '__main__':
    main()
