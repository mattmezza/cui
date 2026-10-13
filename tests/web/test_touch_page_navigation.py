#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Audit mobile page navigation with actual CDP touch contacts.

Known source defects remain strict failures. --allow-defects collects evidence
without returning failure; it does not make the failed behaviors verified.
"""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


class MobileBrowser:
    def __init__(self, browser):
        self.browser = browser

    def new_page(self, **options):
        options.update(viewport={'width': 390, 'height': 844}, is_mobile=True,
                       device_scale_factor=1)
        return self.browser.new_page(**options)


def align_canvas(a):
    a.page.evaluate('scrollTo(0,100)')
    a.page.wait_for_timeout(100)
    assert a.page.evaluate('scrollY') > 50, 'Fixture needs room to scroll upward'
    assert a.page.evaluate('document.scrollingElement.scrollHeight-innerHeight-scrollY') > 50, 'Fixture needs room to scroll downward'


def swipe(a, point, dy):
    a.dispatch('touchStart', [point])
    for step in range(1, 7):
        a.dispatch('touchMove', [{**point, 'y': point['y']+dy*step/6}])
    a.dispatch('touchEnd', [])
    a.page.wait_for_timeout(200)


def canvas_point(a, outside=False):
    rect = a.page.locator('canvas').bounding_box()
    point = {'id': 1, 'x': rect['x']-8 if outside == 'padding' else 8 if outside else rect['x']+rect['width']/2,
             'y': rect['y']+min(rect['height']/2, 170)}
    is_canvas = a.page.evaluate('({x,y})=>document.elementFromPoint(x,y)===document.querySelector("canvas")', point)
    assert is_canvas == (not bool(outside)), 'Touch fixture is occluded or uses wrong DOM surface'
    return point


def page_pan(a, outside=False):
    align_canvas(a)
    before = a.page.evaluate('scrollY')
    swipe(a, canvas_point(a, outside), -110)
    after = a.page.evaluate('scrollY')
    assert after > before+25, f'Page blocked touch pan: window.scrollY {before} -> {after}; touch-action={a.page.locator("canvas").evaluate("e=>getComputedStyle(e).touchAction")}'


def owner(a):
    return a.target(predicate=lambda s: s['f'][17] and s['f'][21] > 40)


def owner_point(a, target):
    rect = a.page.locator('canvas').bounding_box()
    point = {'id': 1, 'x': rect['x']+target['f'][1]+target['f'][3]/2,
             'y': rect['y']+target['f'][2]+target['f'][4]/2}
    assert 160 < point['y'] < 650, f'Internal owner is outside mobile test area: {point}'
    return point


def edge(a, bottom):
    for _ in range(30):
        current = owner(a)
        remaining = current['f'][21]-current['f'][19] if bottom else current['f'][19]
        if remaining < .1:
            return current
        delta = max(20, min(150, remaining)) * (-1 if bottom else 1)
        swipe(a, owner_point(a, current), delta)
    raise AssertionError('Actual touch setup did not reach scroll boundary')


def run_cases(a):
    a.case('Avatar', 'outside-canvas native vertical page pan', lambda: page_pan(a, True))
    a.case('Avatar', 'canvas wrapper padding permits native page pan', lambda: page_pan(a, 'padding'))
    for family in ['Avatar', 'Image', 'Label']:
        a.case(family, 'passive canvas permits vertical page navigation', lambda: page_pan(a))

    def button_ghost():
        align_canvas(a)
        target = a.target('Run action', 1)
        point = a.point(target)
        before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        a.dispatch('touchStart', [point])
        a.dispatch('touchMove', [{**point, 'y': point['y']-35}])
        a.dispatch('touchMove', [point])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before, 'Backtracking touch drag activated button'
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1, 'Later actual tap failed or duplicated activation'
    a.case('Button', 'drag has no ghost activation; later actual tap works exactly once', button_ghost)

    def button_page():
        align_canvas(a)
        before = a.page.evaluate('scrollY')
        clicks = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        swipe(a, a.point(a.target('Run action', 1)), -110)
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks, 'Navigation drag activated button'
        after = a.page.evaluate('scrollY')
        assert after > before+25, f'Button without internal owner blocked page pan: {before} -> {after}; activation count unchanged'
    a.case('Button', 'vertical drag without internal owner navigates page without activating', button_page)

    def internal():
        align_canvas(a)
        current = edge(a, False)
        before = a.page.evaluate('scrollY')
        swipe(a, owner_point(a, current), -110)
        assert abs(a.value(current, 19)-110) < .2, 'Internal owner did not consume logical touch pixels'
        assert abs(a.page.evaluate('scrollY')-before) < 1, 'Consumed internal pan also moved page'
    a.case('Scrollable', 'internal owner consumes touch pan while page stays fixed', internal)

    def boundary(bottom):
        align_canvas(a)
        current = edge(a, bottom)
        before = a.page.evaluate('scrollY')
        offset = current['f'][19]
        swipe(a, owner_point(a, current), -110 if bottom else 110)
        assert abs(a.value(current, 19)-offset) < .2, 'Boundary pan moved internal owner beyond its limit'
        after = a.page.evaluate('scrollY')
        moved = after-before if bottom else before-after
        assert moved > 25, f'Internal {"bottom" if bottom else "top"} boundary trapped page pan: {before} -> {after}; internal offset={offset}'
    a.case('Scrollable', 'bottom boundary falls through to page navigation', lambda: boundary(True))
    a.case('Scrollable', 'top boundary falls through to page navigation', lambda: boundary(False))

    def protected_drag(family, name, role, dx, dy):
        align_canvas(a)
        target = a.target(name, role) if name else a.target(role=role)
        a.point(target)  # Establish DOM visibility before measuring gesture scroll.
        before_page = a.page.evaluate('scrollY')
        before = a.page.evaluate('cuiGallery.exports.gallery_color_rgba()') if family == 'ColorPicker' else target['f'][8]
        a.drag(target, dx, dy)
        assert abs(a.page.evaluate('scrollY')-before_page) < 1, 'Protected drag moved page'
        after = a.page.evaluate('cuiGallery.exports.gallery_color_rgba()') if family == 'ColorPicker' else a.value(target, 8)
        assert after != before, 'Protected drag did not change its control value'
    a.case('Slider', 'protected slider drag changes value without moving page', lambda: protected_drag('Slider', None, 5, 30, -35))
    a.case('Resizable', 'protected divider drag resizes without moving page', lambda: protected_drag('Resizable', 'Resize panels', 40, 24, -35))
    a.case('ColorPicker', 'protected 2D color drag changes color without moving page', lambda: protected_drag('ColorPicker', 'Saturation/value', 5, -25, 30))

    def plot():
        align_canvas(a)
        target = a.target('Projected sine wave', 20)
        point = a.point(target)
        before_page = a.page.evaluate('scrollY')
        before = next(t for t in a.texts() if t.startswith('Domain:'))
        swipe(a, point, -35)
        assert abs(a.page.evaluate('scrollY')-before_page) < 1, 'Opt-in plot pan moved page'
        a.dispatch('touchStart', [{**point, 'x':point['x']-20}, {**point, 'id':2, 'x':point['x']+20}])
        a.dispatch('touchMove', [{**point, 'x':point['x']-40}, {**point, 'id':2, 'x':point['x']+40}])
        a.dispatch('touchEnd', [])
        assert abs(a.page.evaluate('scrollY')-before_page) < 1, 'Opt-in plot pinch moved page'
        assert next(t for t in a.texts() if t.startswith('Domain:')) != before, 'Opt-in plot pinch failed to change domain'
    a.case('Plot', 'opt-in pan and two-contact pinch stay owned by viewport', plot)

    def modal():
        align_canvas(a)
        a.tap(a.target('Edit preferences', 1))
        a.configure('disabled', True)
        try:
            before = a.page.evaluate('scrollY')
            swipe(a, a.point(a.target(role=17), .5, .15), -35)
            assert abs(a.page.evaluate('scrollY')-before) < 1, 'Modal backdrop touch moved page'
            assert a.page.evaluate('cuiGallery.exports.gallery_surface_open(0)') == 1, 'Modal backdrop drag dismissed modal'
            a.tap(a.target('Save changes', 1))
            assert a.page.evaluate('cuiGallery.exports.gallery_surface_open(0)') == 0, 'Modal touch close stopped working'
        finally:
            a.configure('disabled', False)
    a.case('Dialog', 'disabled modal opener/backdrop denies page pan while close still works', modal)

    def multi():
        align_canvas(a)
        point = canvas_point(a)
        before = a.page.evaluate('scrollY')
        a.dispatch('touchStart', [point, {**point, 'id':2, 'x':point['x']+25}])
        a.dispatch('touchMove', [{**point, 'y':point['y']-50}, {**point, 'id':2, 'x':point['x']+25, 'y':point['y']-50}])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('scrollY') == before, 'Multiple contacts scrolled page'
        swipe(a, canvas_point(a), -80)
        assert a.page.evaluate('scrollY') > before+20, 'New single-contact pan remained blocked after multitouch'
    a.case('Avatar', 'multiple contacts suppress page pan; new single contact can navigate', multi)

    def multi_after_claim():
        align_canvas(a)
        point = a.point(a.target('Run action', 1))
        clicks = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        a.dispatch('touchStart', [point])
        first = {**point, 'y':point['y']-35}
        a.dispatch('touchMove', [first])
        second = {**a.point(a.target('Run action', 1)), 'id':2}
        a.dispatch('touchStart', [first, second])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks, 'Second finger after page handoff activated button'
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks+1
    a.case('Button', 'second contact after page claim cannot ghost-activate; new tap remains usable', multi_after_claim)

    def cancel():
        align_canvas(a)
        point = canvas_point(a)
        cancels = a.page.evaluate('cuiGallery.exports.gallery_touch_page_cancels()')
        a.dispatch('touchStart', [point])
        a.dispatch('touchMove', [{**point, 'y':point['y']-30}])
        a.dispatch('touchCancel', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_touch_page_cancels()') == cancels+1, 'Page handoff canceled Core more than once'
        before = a.page.evaluate('scrollY')
        a.page.wait_for_timeout(100)
        assert a.page.evaluate('scrollY') == before, 'Canceled contact kept moving page'
        a.route('Button')
        a.tap(a.target('Run action', 1))
        clicks = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks+1, 'Tap after canceled page pan failed or duplicated'
    a.case('Avatar', 'page pan cancellation stops movement and next button tap works once', cancel)

    def capture_loss():
        align_canvas(a)
        point = a.point(a.target('Run action', 1))
        clicks = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        cancels = a.page.evaluate('cuiGallery.exports.gallery_touch_page_cancels()')
        a.page.evaluate("document.querySelector('canvas').addEventListener('gotpointercapture', e => { window.auditCapturedPointer = e.pointerId; }, {once:true})")
        a.dispatch('touchStart', [point])
        a.dispatch('touchMove', [{**point, 'y':point['y']-2}])
        a.page.evaluate("document.querySelector('canvas').releasePointerCapture(window.auditCapturedPointer)")
        a.dispatch('touchMove', [{**point, 'y':point['y']-3}])
        a.dispatch('touchEnd', [])
        assert a.page.evaluate('cuiGallery.exports.gallery_touch_page_cancels()') == cancels+1, 'Capture loss must cancel Core exactly once'
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks, 'Capture loss caused ghost activation'
        a.tap(a.target('Run action', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks+1, 'Tap after capture loss failed'
    a.case('Button', 'unexpected pointer capture loss cancels once without ghost activation', capture_loss)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report', type=Path, default=Path('/tmp/cui-touch-page-navigation-report.json'))
    parser.add_argument('--allow-defects', action='store_true')
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
            a = Audit(MobileBrowser(browser), f'http://127.0.0.1:{server.server_port}')
            assert a.page.evaluate('cuiGallery.catalog.components.length') == 78
            run_cases(a)
            assert not a.errors, a.errors
            args.report.write_text(json.dumps({'baseline': 'd56820e', 'viewport': [390, 844],
                'transport': 'Chromium mobile CDP touch', 'cases': a.results,
                'page_errors': a.errors}, indent=2)+'\n')
            browser.close()
            failures = [r for r in a.results if r['status'] != 'verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} page navigation cases passed; report: {args.report}')
            if failures and not args.allow_defects:
                raise SystemExit(1)
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
