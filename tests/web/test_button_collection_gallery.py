#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual gallery Button collections: controlled input and rendered joins."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

class GalleryAudit(Audit):
    def __init__(self, browser, url):
        self.page = browser.new_page(viewport={'width': 1280, 'height': 900}, has_touch=True)
        self.page.set_default_timeout(15000)
        self.errors = []
        self.page.on('pageerror', lambda error: self.errors.append(str(error)))
        self.page.goto(url + '/#Button')
        self.page.wait_for_function('window.cuiGallery && cuiGallery.renderedComponent==="Button"')
        self.page.emulate_media(reduced_motion='reduce')
        self.session = self.page.context.new_cdp_session(self.page)
        self.active_touch = False
        self.results = []
        self.inventory = {}



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    results = []
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            for width in (1280, 390):
                a = GalleryAudit(browser, f'http://127.0.0.1:{server.server_port}')
                a.page.set_viewport_size({'width': width, 'height': 900})
                assert a.target('Run action', 1)  # Initial standalone mode remains available.
                assert not any(node['name'] == 'Day' for node in a.snapshot())
                a.configure('button-collection', 1)
                def stat(field):
                    return a.page.evaluate('(f)=>cuiGallery.exports.gallery_button_collection_stat(f)', field)
                def selected(expected):
                    assert [stat(i) for i in range(3)] == expected
                    assert [bool(a.target(name, 1)['f'][6]) for name in ('Day', 'Week', 'Month')] == [bool(v) for v in expected]
                def mouse(name):
                    q = a.point(a.target(name, 1)); a.page.mouse.click(q['x'], q['y'])
                def activation(action, expected):
                    before = stat(3); action()
                    a.page.wait_for_function('(n)=>cuiGallery.exports.gallery_button_collection_stat(3)===n', arg=before+1)
                    selected(expected); a.page.wait_for_timeout(80); assert stat(3) == before+1
                selected([1, 0, 0])
                activation(lambda: mouse('Week'), [0, 1, 0])
                activation(lambda: mouse('Week'), [0, 1, 0])
                activation(lambda: a.tap(a.target('Month', 1)), [0, 0, 1])
                activation(lambda: a.page.keyboard.press('Enter'), [0, 0, 1])
                before = stat(3)
                a.page.keyboard.down('Enter'); a.page.wait_for_function('(n)=>cuiGallery.exports.gallery_button_collection_stat(3)===n', arg=before+1)
                a.page.keyboard.down('Enter'); a.page.keyboard.up('Enter'); a.page.wait_for_timeout(100)
                assert stat(3) == before+1
                a.configure('button-collection', 2)
                activation(lambda: a.tap(a.target('Day', 1)), [1, 0, 1])
                activation(lambda: mouse('Month'), [1, 0, 0])
                point = a.point(a.target('Week', 1)); before = stat(3)
                a.dispatch('touchStart', [point]); a.dispatch('touchCancel', [])
                assert stat(3) == before; selected([1, 0, 0])
                a.configure('disabled', True); before = stat(3)
                assert all(a.target(name, 1)['f'][5] for name in ('Day', 'Week', 'Month'))
                mouse('Week'); a.tap(a.target('Month', 1)); a.page.keyboard.press('Enter'); a.page.wait_for_timeout(100)
                assert stat(3) == before; selected([1, 0, 0])
                a.configure('disabled', False)
                activation(lambda: a.tap(a.target('Week', 1)), [1, 1, 0])
                for mode in (1, 3):
                    a.configure('button-collection', mode)
                    bounds = [a.target(name, 1)['f'][1:5] for name in ('Day', 'Week', 'Month')]
                    commands = a.page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>Array.from({length:22},(_,f)=>e.gallery_command(i,f)));}''')
                    for i, b in enumerate(bounds):
                        border = next(c for c in commands if c[0] == 1 and all(abs(c[j+1]-b[j]) < .5 for j in range(4)))
                        assert border[12] and border[17]
                        assert (border[13] > 0) == (i == 0)
                        assert (border[16] > 0) == (i == 2)
                        if mode == 1:
                            assert (border[14] > 0) == (i == 2) and (border[15] > 0) == (i == 0)
                            if i: assert border[18] == 0 and abs(b[0]-bounds[i-1][0]-bounds[i-1][2]) < .5
                        else:
                            assert (border[14] > 0) == (i == 0) and (border[15] > 0) == (i == 2)
                            if i: assert border[20] == 0 and abs(b[1]-bounds[i-1][1]-bounds[i-1][3]) < .5
                    if args.report:
                        args.report.parent.mkdir(parents=True, exist_ok=True)
                        a.page.locator('canvas').screenshot(path=str(args.report.with_name(f'button-collection-{width}-{mode}.png')))
                    # Sample the actual Canvas renderer: the rounded outer corner
                    # must expose the same background as the pixel just outside.
                    corner = a.page.evaluate("""b=>{const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');const scale=canvas.width/canvas.getBoundingClientRect().width;const pixel=(x,y)=>Array.from(ctx.getImageData(Math.floor(x*scale),Math.floor(y*scale),1,1).data);return [pixel(b[0],b[1]),pixel(b[0]-2,b[1]-2)];}""", bounds[0])
                    assert corner[0][3] == 255 and corner[1][3] == 255
                    assert all(abs(corner[0][i]-corner[1][i]) <= 3 for i in range(3)), corner
                assert not a.errors, a.errors
                results.append({'width': width, 'status': 'verified', 'mouse_touch_keyboard_disabled_cancel_joins': True})
                a.page.close()
            browser.close()
    finally:
        server.shutdown(); server.server_close()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(results, indent=2)+'\n')
    print('Button collection gallery desktop/mobile mouse/touch/keyboard/disabled/cancel/joins PASS')


if __name__ == '__main__':
    main()
