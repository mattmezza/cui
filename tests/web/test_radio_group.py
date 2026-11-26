#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual browser pointer, CDP touch and keyboard RadioGroup regression."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
import json
from pathlib import Path
import tempfile
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


RADIO_MARKS = """() => {
    const e = cuiGallery.exports, text = new TextDecoder();
    const rows = {};
    for (let i = 0; i < e.gallery_semantic_count(); i++) {
        if (e.gallery_semantic(i, 0) !== 4) continue;
        const name = text.decode(new Uint8Array(e.memory.buffer,
            e.gallery_semantic_name(i), e.gallery_semantic_length(i)));
        rows[name] = {bounds: [1, 2, 3, 4].map(f => e.gallery_semantic(i, f)), alpha: 0};
    }
    for (let i = 0; i < e.gallery_count(); i++) {
        if (e.gallery_command(i, 0) !== 5) continue;
        const x = e.gallery_command(i, 1) + e.gallery_command(i, 3) / 2;
        const y = e.gallery_command(i, 2) + e.gallery_command(i, 4) / 2;
        for (const row of Object.values(rows)) {
            const [left, top, width, height] = row.bounds;
            if (x >= left && x < left + width && y >= top && y < top + height)
                row.alpha = Math.max(row.alpha, e.gallery_bitmap(i, 13));
        }
    }
    return rows;
}"""


def check_animated_marks(browser, url):
    page = browser.new_page(viewport={'width': 1280, 'height': 900})
    errors = []
    page.on('pageerror', lambda error: errors.append(str(error)))
    try:
        page.clock.install()
        page.goto(url + '/#Radio')
        page.wait_for_function('window.cuiGallery?.renderedComponent === "Radio"')
        canvas = page.locator('#preview')
        canvas.scroll_into_view_if_needed()
        page.clock.pause_at(page.evaluate('new Date(Date.now() + 500).toISOString()'))

        def alphas():
            rows = page.evaluate(RADIO_MARKS)
            return tuple(rows[name]['alpha'] for name in ('Standard', 'Priority'))

        def select(name):
            votes = page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
            x, y, w, h = page.evaluate(RADIO_MARKS)[name]['bounds']
            bounds = canvas.bounding_box()
            page.mouse.click(bounds['x'] + x + w / 2, bounds['y'] + y + h / 2)
            page.clock.run_for(20)
            assert page.locator('#accessible').get_by_role('radio', name=name).get_attribute('aria-checked') == 'true'
            assert page.locator('#accessible [role=radio][aria-checked=true]').count() == 1
            assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == votes + 1

        assert alphas() == (255, 0)
        select('Priority')
        rising = alphas()
        assert all(0 < value < 255 for value in rising), rising
        assert page.evaluate('cuiGallery.exports.gallery_next_wake()') > 0
        select('Standard')
        page.clock.run_for(55)
        reversed_marks = alphas()
        assert reversed_marks[0] > rising[0] and reversed_marks[1] < rising[1], (rising, reversed_marks)

        page.clock.run_for(1500)
        assert alphas() == (255, 0)
        frame = page.evaluate('cuiGallery.frame')
        page.clock.run_for(200)
        assert page.evaluate('cuiGallery.frame') == frame
        assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0

        page.emulate_media(reduced_motion='reduce')
        page.clock.run_for(40)
        select('Priority')
        assert alphas() == (0, 255)
        assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
        assert not errors, errors
    finally:
        page.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,default=Path('build/web'))
    p.add_argument('--chromium')
    p.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-radio-group.json')
    args=p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else {}))
            a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            def selected(name):
                radios=[s for s in a.snapshot()if s['f'][0]==4]
                assert len(radios)==3 and sum(bool(s['f'][6])for s in radios)==1
                assert sum(s['f'][9]==0 for s in radios)<=1
                assert a.target(name,4)['f'][6]
            def collection():
                group=a.target('Delivery options',42)
                assert group['f'][16]==2 # Vertical orientation.
                selected('Standard')
                for i,name in enumerate(['Standard','Priority','Express unavailable']):
                    node=a.target(name,4)
                    assert node['f'][31]==i+1 and node['f'][32]==3
                assert a.target('Express unavailable',4)['f'][5]
            a.case('Radio','named collection, set positions and disabled item',collection)
            def pointer():
                count=a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
                point=a.point(a.target('Priority',4))
                a.page.mouse.click(point['x'],point['y']);a.page.wait_for_timeout(80)
                selected('Priority')
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')==count+1
                a.page.mouse.click(point['x'],point['y']);a.page.wait_for_timeout(80)
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')==count+1
            a.case('Radio','mouse selects once with immediate coherent siblings',pointer)
            def keyboard():
                for key,name in [('Home','Standard'),('ArrowDown','Priority'),('ArrowDown','Standard'),('End','Priority'),('ArrowUp','Standard')]:
                    a.page.keyboard.press(key);a.page.wait_for_timeout(60);selected(name)
                    assert a.target(name,4)['f'][7]
            a.case('Radio','Home End and arrow roving wrap and skip disabled',keyboard)
            def touch():
                count=a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
                a.tap(a.target('Priority',4));selected('Priority')
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')==count+1
                a.drag(a.target('Standard',4),25,0);selected('Priority')
                point=a.point(a.target('Standard',4));a.dispatch('touchStart',[point]);a.dispatch('touchCancel',[])
                selected('Priority')
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')==count+1
            a.case('Radio','real touch tap, drag ghost suppression and cancellation',touch)
            def disabled():
                a.configure('disabled',True)
                count=a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
                a.tap(a.target('Standard',4));a.page.keyboard.press('Home');a.page.wait_for_timeout(60)
                selected('Priority')
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')==count
                assert all(s['f'][5]for s in a.snapshot()if s['f'][0]==4)
                a.configure('disabled',False)
            a.case('Radio','disabled group rejects touch and keyboard changes',disabled)
            assert not a.errors,a.errors
            a.case('Radio', 'marks crossfade and reverse, settle without frames, and respect reduced motion',
                   lambda: check_animated_marks(browser, f'http://127.0.0.1:{server.server_port}'))
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.write_text(json.dumps(a.results,indent=2)+'\n')
            failures=[r for r in a.results if r['status']!='verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} RadioGroup browser cases passed')
            browser.close()
            if failures:raise SystemExit(1)
    finally:server.shutdown()
if __name__=='__main__':main()
