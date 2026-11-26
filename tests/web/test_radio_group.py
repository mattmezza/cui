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
from playwright.sync_api import TimeoutError as PlaywrightTimeoutError, sync_playwright
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
        const type = e.gallery_command(i, 0);
        if (type !== 0 && type !== 5) continue;
        const side = e.gallery_command(i, 3), commandHeight = e.gallery_command(i, 4);
        const x = e.gallery_command(i, 1) + side / 2;
        const y = e.gallery_command(i, 2) + commandHeight / 2;
        for (const row of Object.values(rows)) {
            const [left, top, width, height] = row.bounds;
            if (!(x >= left && x < left + width && y >= top && y < top + height)) continue;
            if (type === 5)
                row.alpha = Math.max(row.alpha, e.gallery_bitmap(i, 13));
            else if (side === commandHeight && side === 2 * e.gallery_command(i, 9))
                row.background = [5, 6, 7, 8].map(f => e.gallery_command(i, f));
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
        page.emulate_media(color_scheme='light', reduced_motion='no-preference')
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
        before = page.evaluate(RADIO_MARKS)
        endpoints = (before['Standard']['background'], before['Priority']['background'])
        assert endpoints[0] != endpoints[1]
        select('Priority')
        rising = alphas()
        assert all(0 < value < 255 for value in rising), rising
        during = page.evaluate(RADIO_MARKS)
        assert all(during[name]['background'] not in endpoints for name in ('Standard', 'Priority'))
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


def check_tooltip(browser, url):
    page = browser.new_page(viewport={'width': 520, 'height': 740})
    try:
        page.clock.install()
        page.goto(url + '/#Radio')
        page.wait_for_function('window.cuiGallery?.renderedComponent === "Radio"')
        canvas = page.locator('#preview')
        canvas.scroll_into_view_if_needed()
        page.clock.pause_at(page.evaluate('new Date(Date.now() + 500).toISOString()'))

        def panel():
            return page.evaluate("""() => {
                const e = cuiGallery.exports, decoder = new TextDecoder();
                for (let i = 0; i < e.gallery_semantic_count(); ++i) {
                    const name = decoder.decode(new Uint8Array(e.memory.buffer,
                        e.gallery_semantic_name(i), e.gallery_semantic_length(i)));
                    if (name === 'Track your package online') {
                        return {bounds: [1, 2, 3, 4].map(f => e.gallery_semantic(i, f)),
                            tabIndex: e.gallery_semantic(i, 9)};
                    }
                }
                return null;
            }""")

        assert panel() is None
        votes = page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
        x, y, w, h = page.evaluate(RADIO_MARKS)['Priority']['bounds']
        bounds = canvas.bounding_box()
        page.mouse.move(bounds['x'] + x + w / 2, bounds['y'] + y + h / 2)
        page.clock.run_for(180)
        assert panel() is None
        page.clock.run_for(250)
        shown = panel()
        assert shown and shown['tabIndex'] < 0, shown
        px, py, pw, ph = shown['bounds']
        assert 0 <= px < px + pw <= bounds['width'] + .2
        assert 0 <= py < py + ph <= bounds['height'] + .2
        priority = page.locator('#accessible').get_by_role('radio', name='Priority', exact=True)
        assert priority.get_attribute('aria-description') == 'Arrives Wednesday with tracked delivery'
        assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == votes
        page.mouse.move(0, 0)
        page.clock.run_for(80)
        assert panel() is None

        canvas.focus()
        page.keyboard.press('Tab')
        page.clock.run_for(30)
        page.keyboard.press('ArrowDown')
        page.clock.run_for(450)
        assert panel() and priority.get_attribute('aria-checked') == 'true'
        assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == votes + 1
        page.keyboard.press('Escape')
        page.clock.run_for(450)
        assert panel() is None
        assert priority.get_attribute('aria-checked') == 'true'
        assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == votes + 1
    finally:
        page.close()


def check_tab_order(browser, url):
    page = browser.new_page(viewport={'width': 1280, 'height': 900})
    try:
        page.goto(url + '/#Radio')
        page.wait_for_function('window.cuiGallery?.renderedComponent === "Radio"')
        controls = page.locator('#accessible')
        reset = controls.get_by_role('button', name='Reset selection')
        option = page.locator('#radio-tab-order')
        assert reset.count() == 0
        option.check()
        reset.wait_for(state='attached')
        canvas = page.locator('#preview')

        def wait_for_node(name, fields):
            try:
                page.wait_for_function("""({name, fields}) => {
                    const e = cuiGallery.exports, text = new TextDecoder();
                    for (let i = 0; i < e.gallery_semantic_count(); ++i) {
                        const label = text.decode(new Uint8Array(e.memory.buffer,
                            e.gallery_semantic_name(i), e.gallery_semantic_length(i)));
                        if (label === name)
                            return fields.every(([field, value]) =>
                                e.gallery_semantic(i, field) === value);
                    }
                    return false;
                }""", arg={'name': name, 'fields': list(fields.items())}, timeout=3000)
            except PlaywrightTimeoutError as error:
                raise AssertionError(f'{name} never reported semantic fields {fields}') from error

        canvas.focus()
        page.keyboard.press('Tab')
        wait_for_node('Standard', {6: 1, 7: 1, 9: -2, 36: 1, 37: 0})
        wait_for_node('Priority', {6: 0, 7: 0, 9: -2, 36: 1, 37: 1})
        page.keyboard.press('ArrowDown')
        wait_for_node('Priority', {6: 1, 7: 1, 9: -2, 36: 1, 37: 0})
        wait_for_node('Standard', {6: 0, 7: 0, 37: 1})
        page.keyboard.press('Tab')
        wait_for_node('Reset selection', {7: 1, 9: 0, 37: 0})
        page.keyboard.press('Shift+Tab')
        wait_for_node('Priority', {7: 1})
        page.keyboard.press('Tab')
        wait_for_node('Reset selection', {7: 1})
        page.keyboard.press('Enter')
        wait_for_node('Standard', {6: 1, 7: 0, 37: 0})
        assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == 1

        page.locator('#choice-skip-tab').check()
        wait_for_node('Standard', {9: -2, 36: 1, 37: 1})
        canvas.focus()
        page.keyboard.press('Tab')
        wait_for_node('Reset selection', {7: 1})
        option.uncheck()
        reset.wait_for(state='detached')
        page.locator('#choice-skip-tab').uncheck()
        wait_for_node('Standard', {9: 0, 36: 0, 37: 0})
    finally:
        page.close()


def check_styled_cards(browser, url):
    page = browser.new_page(viewport={'width': 1280, 'height': 900})
    try:
        page.emulate_media(color_scheme='light', reduced_motion='reduce')
        page.goto(url + '/#Radio')
        page.wait_for_function('window.cuiGallery?.renderedComponent === "Radio"')
        canvas = page.locator('#preview')
        cards = page.get_by_role('checkbox', name='Styled cards')
        initial = page.evaluate(RADIO_MARKS)
        cards.check()
        page.wait_for_function(
            'document.querySelector("#snippet").textContent.includes("cui_radio_group_present_styled")')
        canvas.scroll_into_view_if_needed()
        rows = page.evaluate(RADIO_MARKS)
        assert rows['Standard']['bounds'][3] >= initial['Standard']['bounds'][3] + 16

        def row_color(name):
            x, y, w, h = rows[name]['bounds']
            return canvas.evaluate("""(canvas, [x,y]) => Array.from(canvas.getContext('2d')
                .getImageData(Math.floor(x), Math.floor(y), 1, 1).data)""", [x + w - 4, y + h / 2])

        first_selected = row_color('Standard')
        first_plain = row_color('Priority')
        assert first_selected != first_plain
        canvas.focus()
        page.keyboard.press('Tab')
        page.keyboard.press('ArrowDown')
        priority = page.locator('#accessible').get_by_role('radio', name='Priority')
        page.wait_for_function("""() => document.querySelector(
            '#accessible [role=radio][aria-label=Priority][aria-checked=true]') !== null""")
        assert priority.get_attribute('aria-description') == 'Arrives Wednesday with tracked delivery'
        assert row_color('Priority') == first_selected and row_color('Standard') == first_plain
        assert page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == 1

        cards.uncheck()
        page.wait_for_function(
            '!document.querySelector("#snippet").textContent.includes("cui_radio_group_present_styled")')
        assert page.evaluate(RADIO_MARKS)['Standard']['bounds'][3] == initial['Standard']['bounds'][3]
        assert priority.get_attribute('aria-checked') == 'true'
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
            def rich_content():
                standard = a.target('Standard', 4)
                priority = a.target('Priority', 4)
                group = a.page.locator('#accessible')
                assert group.get_by_role('radio', name='Priority', exact=True).get_attribute(
                    'aria-description') == 'Arrives Wednesday with tracked delivery'
                assert group.get_by_role('radio', name='Standard', exact=True).get_attribute(
                    'aria-description') == 'Arrives in four business days'
                # The description may wrap. Click the center of its first painted line.
                def description_point(target, *, wraps=False):
                    layout = a.page.evaluate("""({row, title}) => {
                        const e = cuiGallery.exports, decoder = new TextDecoder();
                        const lines = [];
                        for (let i = 0; i < e.gallery_count(); ++i) {
                            if (e.gallery_command(i, 0) !== 2) continue;
                            const text = decoder.decode(new Uint8Array(e.memory.buffer,
                                e.gallery_command_text(i), e.gallery_command_length(i)));
                            const [x,y,w,h] = [1,2,3,4].map(f => e.gallery_command(i, f));
                            if (y >= row[1] && y + h <= row[1] + row[3] &&
                                text !== title)
                                lines.push({text, x, y, w, h});
                        }
                        return lines;
                    }""", {'row': target['f'][1:5], 'title': target['name']})
                    assert layout and layout[0]['text'].startswith('Arrives '), layout
                    if wraps:
                        assert len({line['y'] for line in layout}) > 1, layout
                    x, y, w, h = target['f'][1:5]
                    line = layout[0]
                    rx = (line['x'] + line['w']/2 - x) / w
                    ry = (line['y'] + line['h']/2 - y) / h
                    assert 0 < rx < 1 and 0 < ry < 1
                    return a.point(target, rx, ry)

                count = a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)')
                mouse = description_point(standard)
                a.page.mouse.click(mouse['x'], mouse['y'])
                a.page.wait_for_timeout(60)
                selected('Standard')
                a.dispatch('touchStart', [description_point(priority, wraps=True)])
                a.dispatch('touchEnd', [])
                selected('Priority')
                assert a.page.evaluate('cuiGallery.exports.gallery_radio_stat(0)') == count + 2
            a.case('Radio', 'rich labels paint and expose descriptions inside one option target',
                   rich_content)
            assert not a.errors,a.errors
            a.case('Radio', 'marks crossfade and reverse, settle without frames, and respect reduced motion',
                   lambda: check_animated_marks(browser, f'http://127.0.0.1:{server.server_port}'))
            a.case('Radio', 'delayed hover and keyboard hints preserve descriptions and selection',
                   lambda: check_tooltip(browser, f'http://127.0.0.1:{server.server_port}'))
            a.case('Radio', 'signed tab order follows the roving option and respects Skip Tab',
                   lambda: check_tab_order(browser, f'http://127.0.0.1:{server.server_port}'))
            a.case('Radio', 'styled cards paint the selected row and preserve keyboard selection',
                   lambda: check_styled_cards(browser, f'http://127.0.0.1:{server.server_port}'))
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.write_text(json.dumps(a.results,indent=2)+'\n')
            failures=[r for r in a.results if r['status']!='verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} RadioGroup browser cases passed')
            browser.close()
            if failures:raise SystemExit(1)
    finally:server.shutdown()
if __name__=='__main__':main()
