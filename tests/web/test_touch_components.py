#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Audit gallery touch behavior using real Chromium CDP contacts.

Configuration controls may be changed through DOM inputs. All component taps,
drags, pans and releases below use Input.dispatchTouchEvent, never mouse input
or direct gallery_event calls. Failures are collected so one defect does not
hide the remaining audit. This is representative behavior coverage, not a
claim that 78 families have complete touch parity.
"""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

SNAPSHOT = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_semantic_count()},(_,i)=>({i,id:String(e.gallery_semantic_id(i)),
 name:d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i))),
 f:Array.from({length:36},(_,f)=>e.gallery_semantic(i,f))}));}'''


class Audit:
    def __init__(self, browser, url):
        self.page = browser.new_page(viewport={'width': 1280, 'height': 900}, has_touch=True)
        self.page.set_default_timeout(3000)
        self.errors = []
        self.page.on('pageerror', lambda error: self.errors.append(str(error)))
        self.page.goto(url + '/#Button')
        self.page.wait_for_function('window.cuiGallery && cuiGallery.renderedComponent==="Button"')
        self.page.emulate_media(reduced_motion='reduce')
        self.session = self.page.context.new_cdp_session(self.page)
        self.active_touch = False
        self.results = []
        self.inventory = {}

    def route(self, name):
        self.page.evaluate('(name)=>location.hash=name', name)
        try:
            self.page.wait_for_function('(name)=>cuiGallery.renderedComponent===name', arg=name)
        except Exception as error:
            current = self.page.evaluate('window.cuiGallery?.renderedComponent')
            raise AssertionError(f'Route {name} did not render (current={current}, errors={self.errors})') from error
        self.page.locator('canvas').scroll_into_view_if_needed()
        self.page.wait_for_timeout(70)

    def snapshot(self):
        return self.page.evaluate(SNAPSHOT)

    def texts(self):
        return self.page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))));}''')

    def target(self, name=None, role=None, predicate=None):
        candidates = [s for s in self.snapshot() if (name is None or s['name'] == name)
                      and (role is None or s['f'][0] == role)
                      and (predicate is None or predicate(s))]
        assert candidates, f'Missing semantic target: {name}, role={role}'
        return candidates[0]

    def value(self, target, field):
        current = self.target(target['name'], target['f'][0], lambda s: s['id'] == target['id'])
        return current['f'][field]

    def point(self, target, rx=.5, ry=.5):
        self.page.locator('canvas').scroll_into_view_if_needed()
        for _ in range(3):
            rect = self.page.locator('canvas').bounding_box()
            x = rect['x'] + target['f'][1] + target['f'][3] * rx
            y = rect['y'] + target['f'][2] + target['f'][4] * ry
            top = self.page.evaluate('document.querySelector("header").getBoundingClientRect().bottom+8')
            if y < top:
                # Browser scroll offsets round to pixels; leave clearance instead of
                # repeatedly requesting an ineffective fractional-pixel correction.
                self.page.evaluate('(delta)=>scrollBy({top:delta,behavior:"instant"})', y-top-1)
            elif y > self.page.viewport_size['height'] - 30:
                self.page.evaluate('(delta)=>scrollBy({top:delta,behavior:"instant"})',
                                   y - (self.page.viewport_size['height'] - 30) + 1)
            else:
                assert self.page.evaluate('({x,y})=>document.elementFromPoint(x,y)===document.querySelector("canvas")', {'x': x, 'y': y}), f'Target is occluded by DOM: {target["name"]}'
                return {'id': 1, 'x': x, 'y': y}
        raise AssertionError(f'Touch target is outside browser viewport: {target["name"]}; '
                             f'canvas={rect}, local={target["f"][1:5]}, point={(x,y)}, '
                             f'scroll={self.page.evaluate("window.scrollY")}, header={top}')

    def dispatch(self, kind, points):
        if kind == 'touchCancel' and not self.active_touch:
            return
        self.session.send('Input.dispatchTouchEvent', {'type': kind, 'touchPoints': points})
        self.active_touch = kind not in ('touchEnd', 'touchCancel')
        self.page.wait_for_timeout(50)

    def tap(self, target):
        self.dispatch('touchStart', [self.point(target)])
        self.dispatch('touchEnd', [])

    def drag(self, target, dx, dy, rx=.5, ry=.5):
        start = self.point(target, rx, ry)
        self.dispatch('touchStart', [start])
        for step in range(1, 5):
            self.dispatch('touchMove', [{**start, 'x': start['x']+dx*step/4,
                                        'y': start['y']+dy*step/4}])
        self.dispatch('touchEnd', [])

    def configure(self, name, value):
        self.page.evaluate('''({name,value})=>{const e=document.getElementById(name);
            if(!e)throw Error('Missing configuration '+name);
            if(typeof value==='boolean')e.checked=value;else e.value=String(value);
            e.dispatchEvent(new Event('change',{bubbles:true}));}''', {'name': name, 'value': value})
        self.page.wait_for_timeout(80)

    def case(self, family, behavior, action):
        self.route(family)
        result = {'family': family, 'behavior': behavior, 'status': 'verified'}
        try:
            action()
        except Exception as error:
            result.update(status='failed', detail=str(error))
            # A failed assertion must not leave a contact pressed for later cases.
            self.dispatch('touchCancel', [])
        self.results.append(result)
        print(f'{result["status"].upper()}: {family}: {behavior}' +
              (f' — {result["detail"]}' if 'detail' in result else ''), flush=True)

    def toggle(self, name, role, field=6):
        target = self.target(name, role)
        before = self.value(target, field)
        self.tap(target)
        assert self.value(target, field) != before, 'Tap did not change control state'
        self.tap(self.target(name, role))
        assert self.value(target, field) == before, 'Second tap did not restore state'

    def selected(self, name, role, field=11):
        target = self.target(name, role)
        self.tap(target)
        assert self.value(target, field) == 1, 'Touch target was not selected'

    def scroll(self, family):
        owner = self.target(predicate=lambda s: s['f'][17] and s['f'][21] > 40 and s['f'][4] > 12)
        before = owner['f'][19]
        point = self.point(owner, .5, .5)
        forward = max(0, owner['f'][21] - before)
        delta = min(35, forward) if forward >= before else -min(35, before)
        assert abs(delta) > 1, 'Fixture has no scrollable range for pixel pan'
        self.dispatch('touchStart', [point])
        self.dispatch('touchMove', [{**point, 'y': point['y']-delta}])
        self.dispatch('touchEnd', [])
        after = self.value(owner, 19)
        assert abs(after-before) > 1, 'Touch pan did not move content'
        assert abs(after-before-delta) < .2, f'Expected pixel pan {delta}, got {after-before}'
        return owner

    def overlay(self, family, opener, slot, closer=None):
        self.tap(self.target(opener, 1))
        assert self.page.evaluate('(slot)=>cuiGallery.exports.gallery_surface_open(slot)', slot) == 1, 'Touch did not open overlay'
        if closer:
            self.tap(self.target(closer, 1))
        else:
            rect = self.page.locator('canvas').bounding_box()
            self.dispatch('touchStart', [{'id': 1, 'x': rect['x']+rect['width']-3,
                                         'y': rect['y']+rect['height']-3}])
            self.dispatch('touchEnd', [])
        assert self.page.evaluate('(slot)=>cuiGallery.exports.gallery_surface_open(slot)', slot) == 0, 'Touch did not dismiss overlay'


def run_cases(a):
    def button():
        before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1
        target = a.target('Run action', 1)
        point = a.point(target)
        a.dispatch('touchStart', [point])
        a.dispatch('touchMove', [{**point, 'y': point['y']+15}])
        a.dispatch('touchMove', [point])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1, 'Touch drag generated ghost action'
        a.configure('disabled', True)
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1, 'Disabled button activated'
        a.configure('disabled', False)
    a.case('Button', 'tap; backtracking drag suppresses ghost tap; disabled tap', button)
    a.case('Checkbox', 'two taps toggle checked state', lambda: a.toggle('Receive updates', 2))
    a.case('Switch', 'two taps toggle checked state', lambda: a.toggle('Notifications', 3))
    a.case('Toggle', 'two taps toggle pressed state', lambda: a.toggle('Pin this item', 1))
    a.case('Radio', 'tap selects Priority', lambda: a.selected('Priority', 4, 6))
    a.case('Tabs', 'tap selects Activity', lambda: a.selected('Activity', 8))
    a.case('Accordion', 'tap expands Configuration', lambda: a.selected('Configuration', 1, 10))
    a.case('Collapsible', 'tap expands Show details', lambda: a.selected('Show details', 1, 10))
    for family, opener, slot, closer in [('Dialog', 'Edit preferences', 0, 'Save changes'),
                                        ('AlertDialog', 'Delete workspace', 1, 'Keep workspace'),
                                        ('Sheet', 'Open settings sheet', 2, 'Close'),
                                        ('Popover', 'Open account card', 3, None)]:
        a.case(family, 'touch opens and dismisses overlay', lambda f=family,o=opener,s=slot,c=closer: a.overlay(f,o,s,c))
    def nested_modal():
        a.tap(a.target('Edit preferences', 1))
        owner = a.target(predicate=lambda s: s['f'][17] and s['f'][21] > 40)
        before = owner['f'][19]
        a.tap(a.target('Advanced preferences', 1))
        control = a.target('Preview releases', 2)
        checked = control['f'][6]
        a.drag(control, 0, -30)
        assert a.value(owner, 19) == before, 'Nested modal touch moved obscured parent scroll owner'
        assert a.value(control, 6) == checked, 'Nested modal drag generated ghost checkbox tap'
        a.tap(a.target('Preview releases', 2))
        assert a.value(control, 6) != checked, 'Nested modal checkbox did not accept touch'
    a.case('Dialog', 'nested modal blocks parent scrolling and preserves drag-vs-tap', nested_modal)
    def select():
        a.tap(a.target('Delivery'))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
        a.tap(a.target('Priority', 14))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Selection did not close popup'
        a.tap(a.target('Delivery', 1))
        assert a.target('Priority', 14)['f'][11] == 1, 'Selected option was not Priority'
        a.tap(a.target('Priority', 14))
    a.case('Select', 'touch opens popup, selects Priority, closes popup', select)
    def slider():
        target = a.target(role=5)
        before = target['f'][8]
        a.drag(target, 40, 0)
        assert a.value(target, 8) != before, 'Touch drag did not change slider value'
    a.case('Slider', 'horizontal touch drag changes value', slider)
    def resizable():
        before = a.page.evaluate('cuiGallery.exports.gallery_resizable_size(0)')
        a.drag(a.target('Resize panels', 40), 24, 0)
        assert a.page.evaluate('cuiGallery.exports.gallery_resizable_size(0)') > before+15
        a.configure('resizable-disabled', True)
        locked = a.page.evaluate('cuiGallery.exports.gallery_resizable_size(0)')
        a.drag(a.target('Resize panels', 40), 24, 0)
        assert a.page.evaluate('cuiGallery.exports.gallery_resizable_size(0)') == locked
        a.configure('resizable-disabled', False)
    a.case('Resizable', 'touch resizes panel; disabled resize remains locked', resizable)
    def textarea_pan():
        a.configure('textarea-rows', 1)
        a.scroll('Textarea')
        a.configure('textarea-rows', 4)
    a.case('Textarea', 'vertical pan consumes pixels in overflowing one-row fixture', textarea_pan)
    def editor_pan():
        a.tap(a.target('C code editor', 6))
        a.page.keyboard.press('Control+End')
        a.page.wait_for_function('cuiGallery.exports.gallery_editor_stat(0)===cuiGallery.exports.gallery_editor_stat(2)')
        for _ in range(30):
            before = a.page.evaluate('cuiGallery.exports.gallery_editor_stat(2)')
            a.page.keyboard.press('Enter')
            # Keyboard dispatch finishes before the asynchronous C frame. Measure
            # the pan only after every fixture edit has reached the real model.
            a.page.wait_for_function('(before)=>cuiGallery.exports.gallery_editor_stat(2)>before', arg=before)
        a.scroll('Editor')
    a.case('Editor', 'vertical pan consumes pixels after keyboard-created overflow', editor_pan)
    for family in ['Scrollable', 'VirtualList', 'Sidebar', 'MessageScroller', 'DataTable']:
        if family == 'MessageScroller':
            def conversation_pan():
                owner = a.target('Conversation', 38)
                row = a.target(role=37, predicate=lambda s: owner['f'][2]+30 < s['f'][2] < owner['f'][2]+150)
                point = a.point(owner, .1, .3)
                a.dispatch('touchStart', [point])
                # The first move measures newly visible virtual rows. Assert the
                # next move against displayed geometry, independent of estimates.
                a.dispatch('touchMove', [{**point, 'y': point['y']+60}])
                row = a.target(row['name'], 37)
                before = a.target('Conversation', 38)['f'][19]
                a.dispatch('touchMove', [{**point, 'y': point['y']+90}])
                a.dispatch('touchEnd', [])
                assert abs(a.target(row['name'], 37)['f'][2]-row['f'][2]-30) < .2
                assert a.target('Conversation', 38)['f'][19] < before
            a.case(family, 'second virtual pan moves the measured visible row by exact pixels', conversation_pan)
        else:
            a.case(family, 'vertical pan consumes logical pixels', lambda f=family: a.scroll(f))
    def virtual():
        before = [s['name'] for s in a.snapshot() if s['f'][0] == 37]
        owner = a.scroll('VirtualList')
        a.drag(owner, 0, -120)
        after = [s['name'] for s in a.snapshot() if s['f'][0] == 37]
        assert before != after and len(after) <= 10, 'Touch pan did not update bounded virtual row window'
    a.case('VirtualList', 'touch changes bounded visible row window', virtual)
    def table_selection():
        owner = a.target(predicate=lambda s: s['f'][17] and s['f'][21] > 40)
        cell = a.target(role=30, predicate=lambda s: s['f'][2] >= owner['f'][2] and not s['f'][11])
        a.tap(cell)
        assert a.value(cell, 11) == 1, 'Touched grid cell was not selected'
    a.case('DataTable', 'touch selects actual visible grid cell', table_selection)
    a.case('Sidebar', 'touch selects Files', lambda: a.selected('Files', 23))
    a.case('List', 'touch selects Priority option', lambda: a.selected('Priority', 14))
    def stepper():
        review = a.target('Review', 1)
        a.tap(review)
        assert a.value(review, 11) == 0, 'Unconfirmed navigation bypassed owner validation'
        confirmation = a.target('Details confirmed')
        a.tap(confirmation)
        assert a.value(confirmation, 6) == 1, 'Touch did not confirm details'
        a.selected('Review', 1)
    a.case('Stepper', 'touch rejects unconfirmed navigation, then selects Review', stepper)
    def rating():
        a.tap(a.target('5 of 5', 1))
        assert a.target(role=5)['f'][8] == 5, 'Touch did not change rating'
    a.case('Rating', 'touch selects five-star rating', rating)
    def pagination():
        before = a.target('Pagination', 9)['f'][8]
        a.tap(a.target('Next', 1))
        assert a.target('Pagination', 9)['f'][8] == before+1, 'Touch did not advance page'
    a.case('Pagination', 'touch advances current page', pagination)
    def calendar():
        day = a.target(role=1, predicate=lambda s: s['name'].endswith('-25'))
        a.tap(day)
        assert a.value(day, 11) == 1, 'Touch did not select date'
    a.case('Calendar', 'touch selects calendar day', calendar)
    def list_disabled():
        before = [(s['id'], s['f'][11]) for s in a.snapshot() if s['f'][0] == 14]
        a.tap(a.target('Unavailable', 14))
        after = [(s['id'], s['f'][11]) for s in a.snapshot() if s['f'][0] == 14]
        assert before == after, 'Disabled option changed selection'
    a.case('List', 'disabled option preserves selection', list_disabled)
    def dock():
        a.tap(a.target('Reset layout', 1))
        tab = a.target('Panel 2', 8)
        area = a.target('Dock area', 9)
        start = a.point(tab)
        start['x'] -= 12
        end = a.point(area, .99, .5)
        a.dispatch('touchStart', [start])
        for step in range(1, 9):
            a.dispatch('touchMove', [{**start, 'x': start['x']+(end['x']-start['x'])*step/8,
                                     'y': start['y']+(end['y']-start['y'])*step/8}])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_dock_panel_region(2)') == 2, 'Touch did not move panel to right edge'
        assert a.page.evaluate('cuiGallery.exports.gallery_dock_stat(1)') == 1, 'Moved panel edge was not opened'
        a.tap(a.target('Reset layout', 1))
    a.case('Dock', 'touch drags tab from center into right edge', dock)
    def carousel():
        before = a.page.evaluate('cuiGallery.exports.gallery_carousel_stats(0)')
        target = a.target('Workflow carousel')
        a.drag(target, -target['f'][3]*.65, 0)
        assert a.page.evaluate('cuiGallery.exports.gallery_carousel_stats(0)') != before, f'Touch swipe did not change carousel page; stats={[a.page.evaluate("(f)=>cuiGallery.exports.gallery_carousel_stats(f)", f) for f in range(4)]}'
    a.case('Carousel', 'horizontal touch swipe changes page', carousel)
    def menu():
        a.tap(a.target('Actions', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1, 'Touch did not open menu'
        a.tap(a.target('Priority', 16))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Touch did not close chosen menu'
    a.case('Menu', 'touch opens menu and invokes option', menu)
    def hover(expected):
        a.tap(a.target('Hover or focus here', 1))
        a.page.wait_for_timeout(400)
        assert expected in a.texts(), 'Touch focus did not reveal floating content'
    a.case('Tooltip', 'touch focus reveals tooltip content', lambda: hover('A real CUI floating tooltip'))
    a.case('HoverCard', 'touch focus reveals profile card', lambda: hover('Alex Morgan'))
    def plot():
        target = a.target('Projected sine wave', 20)
        before = next(t for t in a.texts() if t.startswith('Domain:'))
        point = a.point(target)
        a.dispatch('touchStart', [{**point, 'x': point['x']-25}, {**point, 'id': 2, 'x': point['x']+25}])
        a.dispatch('touchMove', [{**point, 'x': point['x']-50}, {**point, 'id': 2, 'x': point['x']+50}])
        a.dispatch('touchEnd', [])
        assert next(t for t in a.texts() if t.startswith('Domain:')) != before, 'Opt-in plot pinch did not change domain'
    a.case('Plot', 'two actual touch contacts zoom opted-in viewport', plot)
    def input_focus():
        target = a.target(role=6, predicate=lambda s: not s['f'][5])
        a.tap(target)
        assert a.value(target, 7) == 1, 'Touch did not focus editable control'
    for family in ['Input', 'OtpInput', 'NumberInput', 'TimeField']:
        a.case(family, 'touch focuses editable control (text/IME not asserted)', input_focus)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report', type=Path, default=Path('/tmp/cui-touch-components-report.json'))
    parser.add_argument('--inventory-only', action='store_true')
    parser.add_argument('--allow-defects', action='store_true', help='Audit/report mode; default fails on any case failure')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
            names = a.page.evaluate('cuiGallery.catalog.components.map(c=>c.name)')
            assert len(names) == 78 and len(set(names)) == 78
            for name in names:
                a.route(name)
                assert a.page.locator('#preview-card').is_visible(), name
                a.inventory[name] = a.snapshot()
            if not args.inventory_only:
                run_cases(a)
            assert not a.errors, a.errors
            report = {'baseline': '65da7b2', 'transport': 'Chromium CDP Input.dispatchTouchEvent',
                      'families': a.inventory, 'cases': a.results, 'page_errors': a.errors}
            args.report.write_text(json.dumps(report, indent=2)+'\n')
            browser.close()
            failures = [r for r in a.results if r['status'] != 'verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} touch cases passed; all78 stories rendered; report: {args.report}')
            if failures and not args.allow_defects:
                raise SystemExit(1)
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
