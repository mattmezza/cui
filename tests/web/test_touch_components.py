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
        a.configure('dialog-long-body', True)
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
    def live_virtual_inbox():
        def visible_rows():
            owner = a.target('Scrollable content', 9)
            top = owner['f'][2]
            rows = [r for r in a.snapshot()
                    if r['f'][0] == 37 and r['f'][2] < top + owner['f'][4]
                    and r['f'][2] + r['f'][4] > top]
            return owner, sorted(rows, key=lambda r: r['f'][31])

        a.configure('virtual-axis', 2)
        try:
            assert 'cui_virtual_restore_anchor' in a.page.locator('#snippet').text_content()
            owner, before = visible_rows()
            first, successor = before[:2]
            assert len(before) < 8 and first['f'][4] != successor['f'][4]
            relative = first['f'][2] - owner['f'][2]
            a.tap(a.target('Prepend message', 1))
            updated, rows = visible_rows()
            assert rows[0]['id'] == first['id'] and rows[0]['f'][31] == first['f'][31] + 1
            assert rows[0]['f'][32] == first['f'][32] + 1
            assert abs(rows[0]['f'][2] - updated['f'][2] - relative) < .2
            assert abs(updated['f'][19] - owner['f'][19] - 62) < .2
            owner = updated

            a.tap(a.target('Resize row above', 1))
            updated, rows = visible_rows()
            assert rows[0]['id'] == first['id'] and rows[0]['f'][32] == first['f'][32] + 1
            assert abs(rows[0]['f'][2] - updated['f'][2] - relative) < .2
            assert abs(updated['f'][19] - owner['f'][19] - 32) < .2

            a.tap(a.target('Remove first visible', 1))
            updated, rows = visible_rows()
            assert rows[0]['id'] == successor['id'] and rows[0]['f'][32] == first['f'][32]
            assert abs(rows[0]['f'][2] - updated['f'][2]) < .2
            assert all(r['id'] != first['id'] for r in a.snapshot())

            a.page.set_viewport_size({'width': 390, 'height': 844})
            a.configure('disabled', True)
            prepend = a.target('Prepend message', 1)
            remove = a.target('Remove first visible', 1)
            assert prepend['f'][5] and remove['f'][5] and remove['f'][2] > prepend['f'][2]
            canvas = a.page.locator('canvas').bounding_box()
            assert remove['f'][1] + remove['f'][3] <= canvas['width']
            a.tap(prepend)
            assert visible_rows()[1][0]['f'][32] == first['f'][32]
        finally:
            a.page.set_viewport_size({'width': 1280, 'height': 900})
            a.configure('disabled', False)
            a.configure('virtual-axis', 0)
    a.case('VirtualList', 'live inbox preserves anchor on prepend and resize and reveals successor on removal',
           live_virtual_inbox)
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
    def narrow_pagination():
        viewport = a.page.viewport_size
        try:
            a.page.set_viewport_size({'width': 390, 'height': viewport['height']})
            a.configure('design-size', 7)  # 2xl
            assert a.page.locator('#design-size').is_visible()
            first, last = a.target('First', 1), a.target('Last', 1)
            assert first['f'][4] == 56 and first['f'][2] < last['f'][2]
            assert 'CUI_DESIGN_2XL' in a.page.locator('#snippet').text_content()
            before = int(a.target('Pagination', 9)['f'][8])
            assert a.texts().count(f'{before} / 20') == 1, 'Compact counter split across lines'
            a.tap(a.target('Next', 1))
            assert a.target('Pagination', 9)['f'][8] == before + 1
            assert a.texts().count(f'{before + 1} / 20') == 1

            a.configure('design-size', 14)  # 9xl keeps natural item widths.
            assert a.target('Previous', 1)['f'][4] == 264
            canvas_width = a.page.locator('canvas').evaluate('e => e.clientWidth')
            assert a.page.locator('canvas').evaluate('e => e.parentElement.clientWidth') < canvas_width
            for label in ('First', 'Previous', 'Next', 'Last'):
                button = a.target(label, 1)['f']
                assert button[1] + button[3] <= canvas_width - 24, label
            assert a.texts().count(f'{before + 1} / 20') == 1
        finally:
            a.page.set_viewport_size(viewport)
    a.case('Pagination', '2xl controls wrap at 390px and keep an actionable, single-line counter',
           narrow_pagination)
    def pagination_modes():
        a.configure('design-size', 3)
        a.tap(a.target('First', 1))
        a.configure('pagination-mode', 1)
        assert a.page.locator('#pagination-visible-pages').is_visible()
        def page_buttons():
            return [s for s in a.snapshot() if s['f'][0] == 1 and s['name'].isdigit()]
        assert len(page_buttons()) == 5
        a.configure('pagination-visible-pages', 10)
        assert len(page_buttons()) == 10
        assert '.visible_pages = 10' in a.page.locator('#snippet').text_content()
        a.tap(a.target('8', 1))
        assert a.target('Pagination', 9)['f'][8] == 8
        a.configure('pagination-mode', 2)
        assert a.page.locator('#pagination-visible-pages').is_hidden()
        assert [s['name'] for s in a.snapshot() if s['f'][0] == 1] == ['Previous', 'Next']
        assert '.icon_only = true' in a.page.locator('#snippet').text_content()
        assert a.page.evaluate('''() => {
            const e = cuiGallery.exports;
            return Array.from({length: e.gallery_count()}, (_, i) => e.gallery_command(i, 0))
                .filter(type => type === 5).length;
        }''') == 2, 'Both arrow icons must draw as images'
        a.tap(a.target('Previous', 1))
        assert a.target('Pagination', 9)['f'][8] == 7
        a.configure('disabled', True)
        a.tap(a.target('Next', 1))
        assert a.target('Pagination', 9)['f'][8] == 7
        a.configure('disabled', False)
    a.case('Pagination', 'visible-page budgets and icon-only controls support touch', pagination_modes)
    def pagination_ellipsis():
        a.configure('design-size', 3)
        a.configure('pagination-mode', 0)
        a.tap(a.target('First', 1))
        a.configure('pagination-mode', 1)
        a.configure('pagination-visible-pages', 5)
        a.tap(a.target('Later pages', 1))
        assert a.target('Later pages', 1)['f'][10] == 1
        popup = a.target('Later pages', 15)
        assert popup['f'][17] and popup['f'][4] <= 240
        assert a.target('5', 16)['f'][7] == 1
        a.drag(popup, 0, -92)
        assert a.target('Later pages', 15)['f'][19] > 0
        assert a.target('Pagination', 9)['f'][8] == 1, 'Pan must not select a page'
        visible = a.target(role=16, predicate=lambda s: int(s['name']) > 5 and
                           s['f'][2] + s['f'][4] <= popup['f'][2] + popup['f'][4] - 4)
        a.tap(visible)
        assert a.target('Pagination', 9)['f'][8] == int(visible['name'])
        assert all(s['f'][0] != 15 for s in a.snapshot())
        assert '.ellipsis_viewport = viewport' in a.page.locator('#snippet').text_content()
    a.case('Pagination', 'touch opens and scrolls an ellipsis menu before jumping to a page',
           pagination_ellipsis)
    def pagination_localized_chevrons():
        viewport = a.page.viewport_size
        try:
            a.page.set_viewport_size({'width': 390, 'height': viewport['height']})
            a.configure('pagination-mode', 0)
            a.tap(a.target('First', 1))
            a.configure('design-size', 7)  # 2xl
            a.configure('pagination-mode', 3)
            a.configure('pagination-language', 'fr')
            assert a.page.locator('#pagination-visible-pages').is_visible()
            names = [s['name'] for s in a.snapshot() if s['f'][0] == 1]
            assert 'Première' not in names and 'Dernière' not in names
            prev, next_page = a.target('Précédente', 1), a.target('Suivante', 1)
            assert prev['f'][2] < next_page['f'][2] and next_page['f'][4] == 56
            assert 'Précédente' in a.texts() and 'Suivante' in a.texts()
            snippet = a.page.locator('#snippet').text_content()
            assert 'CUI_PAGINATION_NAV_CHEVRONS' in snippet and '.labels = &labels' in snippet
            a.tap(next_page)
            assert a.target('Pages de résultats', 9)['f'][8] == 2
            a.tap(a.target('Pages suivantes', 1))
            assert a.target('Pages suivantes', 15)['f'][17] == 1
            a.page.locator('canvas').press('Escape')
            a.configure('design-size', 14)  # 9xl preserves the whole caption and chevron.
            canvas = a.page.locator('canvas')
            canvas_width = canvas.evaluate('e => e.clientWidth')
            assert canvas.evaluate('e => e.parentElement.clientWidth') < canvas_width
            for label in ('Précédente', 'Suivante'):
                bounds = a.target(label, 1)['f']
                assert bounds[1] + bounds[3] <= canvas_width - 24, label
                assert label in a.texts()
        finally:
            a.page.set_viewport_size(viewport)
    a.case('Pagination', 'translated chevrons and jump menus work at 390px',
           pagination_localized_chevrons)
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
