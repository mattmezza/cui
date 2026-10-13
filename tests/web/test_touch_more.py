#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Follow-up touch audit; component actions use actual CDP touch contacts.

DOM configuration creates fixtures. Keyboard setup, where stated, prepares a
query; it does not count as touch text/IME verification. The imported helper
collects failures without leaving a pressed contact behind.
"""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

FAMILIES = ['DropdownButton', 'Combobox', 'Command', 'Calendar', 'DatePicker',
            'ColorPicker', 'Clipboard', 'Toggle', 'Tree', 'Focus Trap']


def stat(a, field):
    return a.page.evaluate('(f)=>cuiGallery.exports.gallery_command_palette_stats(f)', field)


def outside(a):
    rect = a.page.locator('canvas').bounding_box()
    a.dispatch('touchStart', [{'id': 1, 'x': rect['x']+rect['width']-3,
                              'y': rect['y']+rect['height']-3}])
    a.dispatch('touchEnd', [])


def run_cases(a):
    def dropdown():
        before = a.page.evaluate('cuiGallery.exports.gallery_clicks()')
        a.tap(a.target('Export workspace', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1
        a.tap(a.target('Export options', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
        a.tap(a.target('Priority', 16))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
        a.tap(a.target('Export options', 1))
        outside(a)
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Outside touch did not dismiss split menu'
        a.configure('disabled', True)
        try:
            a.tap(a.target('Export workspace', 1))
            a.tap(a.target('Export options', 1))
            assert a.page.evaluate('cuiGallery.exports.gallery_clicks()') == before+1
            assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
        finally:
            a.configure('disabled', False)
    a.case('DropdownButton', 'primary action, touch menu choice/outside dismissal, disabled controls', dropdown)

    def combobox():
        a.tap(a.target('Find a fruit', 6))
        a.page.keyboard.type('Ban')  # query fixture; the asserted selection uses touch
        a.page.wait_for_timeout(100)
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
        a.tap(a.target('Banana', 14))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Touched suggestion did not close popup'
        a.tap(a.target('Find a fruit', 6))
        a.page.keyboard.press('End')
        a.page.keyboard.type('a')
        a.page.wait_for_timeout(100)
        assert a.target('Banana', 14)['f'][11] == 1, 'Touched suggestion did not become selected option'
        outside(a)
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Outside touch did not dismiss combobox'
    a.case('Combobox', 'touch focuses query and chooses matching suggestion (keyboard query setup)', combobox)

    def combobox_disabled():
        a.configure('disabled', True)
        try:
            target = a.target('Find a fruit', 6)
            assert target['f'][5] == 1, 'Combobox did not expose disabled state'
            a.tap(target)
            assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Disabled combobox opened suggestions'
        finally:
            a.configure('disabled', False)
    a.case('Combobox', 'disabled touch does not open suggestion popup', combobox_disabled)

    def command():
        before = stat(a, 2)
        a.tap(a.target('Open file', 14))
        assert stat(a, 2) == before+1 and stat(a, 3) >= 1, 'Touch did not confirm and invoke command'
        a.tap(a.target('Disabled action', 14))
        assert stat(a, 2) == before+1, 'Disabled command confirmed'
    a.case('Command', 'touch confirms enabled command and ignores disabled command', command)
    def command_drag():
        target = a.target('Open file', 14)
        before = stat(a, 2)
        point = a.point(target)
        a.dispatch('touchStart', [point])
        a.dispatch('touchMove', [{**point, 'y': point['y']+15}])
        a.dispatch('touchMove', [point])
        a.dispatch('touchEnd', [])
        assert stat(a, 2) == before, 'Command row backtracking drag generated ghost confirmation'
    a.case('Command', 'touch row drag backtracks without ghost confirmation', command_drag)

    def command_modal():
        a.configure('command-modal', True)
        try:
            a.tap(a.target('Open command palette', 1))
            assert stat(a, 8) == 1
            before = stat(a, 2)
            a.tap(a.target('Open file', 14))
            assert stat(a, 2) == before+1, 'Modal command did not confirm'
            outside(a)
            assert stat(a, 8) == 0, 'Outside touch did not dismiss command surface'
        finally:
            a.configure('command-modal', False)
    a.case('Command', 'touch opens modal palette, confirms command and dismisses outside', command_modal)

    def calendar():
        target = a.target(role=1, predicate=lambda s: s['name'].endswith('-20'))
        a.tap(target)
        assert a.value(target, 11) == 1, 'Touched date was not selected'
        a.configure('disabled', True)
        try:
            other = a.target(role=1, predicate=lambda s: s['name'].endswith('-21'))
            a.tap(other)
            assert a.value(target, 11) == 1 and a.value(other, 11) == 0, 'Disabled calendar changed date'
        finally:
            a.configure('disabled', False)
    a.case('Calendar', 'touch selects day; disabled day does not change selection', calendar)
    def date_picker():
        a.tap(a.target('Date', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
        day = a.target(role=1, predicate=lambda s: s['name'].endswith('-22'))
        a.tap(day)
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
        assert any(day['name'] in t for t in a.texts()), 'Selected date not reflected in trigger'
        a.tap(a.target('Date', 1))
        outside(a)
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
        a.configure('disabled', True)
        try:
            a.tap(a.target('Date', 1))
            assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0, 'Disabled date picker opened'
        finally:
            a.configure('disabled', False)
    a.case('DatePicker', 'touch opens/selects date; outside dismissal and disabled trigger', date_picker)

    def color():
        before = a.page.evaluate('cuiGallery.exports.gallery_color_rgba()')
        a.drag(a.target('Red', 5), 35, 0)
        after = a.page.evaluate('cuiGallery.exports.gallery_color_rgba()')
        assert before != after, 'Touch channel drag did not change RGBA'
        a.configure('disabled', True)
        try:
            a.drag(a.target('Red', 5), 35, 0)
            assert a.page.evaluate('cuiGallery.exports.gallery_color_rgba()') == after, 'Disabled color changed'
        finally:
            a.configure('disabled', False)
    a.case('ColorPicker', 'touch channel slider changes RGBA; disabled drag preserves color', color)

    def color_palette():
        a.tap(a.target('#5865F2FF', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_color_rgba()') == 0x5865F2FF, 'Palette touch did not select exact RGBA'
        before = a.page.evaluate('cuiGallery.exports.gallery_color_rgba()')
        a.drag(a.target('Saturation/value', 5), -30, 20)
        assert a.page.evaluate('cuiGallery.exports.gallery_color_rgba()') != before, 'Touch 2D saturation/value drag did not change color'
    a.case('ColorPicker', 'touch palette selects exact RGBA and 2D saturation/value drag changes color', color_palette)

    def clipboard():
        a.page.context.grant_permissions(['clipboard-read', 'clipboard-write'])
        a.page.evaluate('navigator.clipboard.writeText("touch sentinel")')
        a.tap(a.target('Copy text', 1))
        a.page.wait_for_timeout(150)
        assert a.page.evaluate('navigator.clipboard.readText()') == 'Hello from CUI', 'Touch copy did not update actual clipboard'
        a.tap(a.target('Read text', 1))
        a.page.wait_for_timeout(150)
        assert 'Read: Hello from CUI' in a.texts(), 'Touch read did not expose actual clipboard contents'
        a.configure('disabled', True)
        try:
            a.page.evaluate('navigator.clipboard.writeText("disabled sentinel")')
            a.tap(a.target('Copy text', 1))
            assert a.page.evaluate('navigator.clipboard.readText()') == 'disabled sentinel', 'Disabled copy changed clipboard'
        finally:
            a.configure('disabled', False)
    a.case('Clipboard', 'trusted touch copy/read actual clipboard; disabled copy preserves sentinel', clipboard)

    def toggles():
        a.toggle('Pin this item', 1)
        a.configure('disabled', True)
        try:
            before = a.target('Pin this item', 1)['f'][6]
            a.tap(a.target('Pin this item', 1))
            assert a.target('Pin this item', 1)['f'][6] == before, 'Disabled standalone toggle changed'
        finally:
            a.configure('disabled', False)
    a.case('Toggle', 'standalone two taps and disabled tap preserve expected state', toggles)

    def single_group():
        a.configure('toggle-selection', 0)
        a.tap(a.target('Italic', 1))
        a.page.wait_for_timeout(150)
        model = [a.page.evaluate('(f)=>cuiGallery.exports.gallery_control_stats(f)', f) for f in range(4,7)]
        semantic = [a.target(n, 1)['f'][6] for n in ['Bold', 'Italic', 'Underline']]
        assert model == [0, 1, 0], f'Single selection model {model}'
        assert semantic == model, f'Single selection semantic states {semantic}; model {model}'
    a.case('Toggle', 'controlled single group selection updates model and checked semantics', single_group)

    def multiple_group():
        a.configure('toggle-selection', 1)
        a.tap(a.target('Bold', 1))
        assert a.target('Bold', 1)['f'][6] == 1 and a.target('Italic', 1)['f'][6] == 1, 'Multiple mode did not retain both selected toggles'
        a.configure('disabled', True)
        try:
            before = [a.target(n, 1)['f'][6] for n in ['Bold', 'Italic', 'Underline']]
            a.tap(a.target('Underline', 1))
            assert before == [a.target(n, 1)['f'][6] for n in ['Bold', 'Italic', 'Underline']], 'Disabled group changed state'
        finally:
            a.configure('disabled', False)
    a.case('Toggle', 'controlled multiple group retains both selections and ignores disabled tap', multiple_group)

    def tree():
        before = len([s for s in a.snapshot() if s['f'][0] == 23])
        a.tap(a.target('Expand', 1))
        assert a.target('docs', 23)['f'][10] == 1
        assert len([s for s in a.snapshot() if s['f'][0] == 23]) > before, 'Tree expansion did not expose child'
        a.tap(a.target('core.c', 23))
        assert a.target('core.c', 23)['f'][11] == 1, 'Touch did not select leaf'
        a.configure('disabled', True)
        try:
            a.tap(a.target('base.c', 23))
            assert a.target('core.c', 23)['f'][11] == 1 and a.target('base.c', 23)['f'][11] == 0, 'Disabled tree changed selected leaf'
        finally:
            a.configure('disabled', False)
        src = a.target('src', 23)
        a.tap(a.target('Collapse', 1, lambda s: src['f'][2] <= s['f'][2] < src['f'][2]+src['f'][4]))
        assert a.target('src', 23)['f'][10] == 0
        assert not any(s['name'] == 'core.c' for s in a.snapshot()), 'Collapsed branch retained child row'
    a.case('Tree', 'touch expands/collapses branch, selects leaf and ignores disabled leaf', tree)

    def focus_trap():
        a.tap(a.target('Open focus trap', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 1
        a.tap(a.target('First control', 1))
        assert a.target('First control', 1)['f'][7] == 1, 'Touch did not focus trapped control'
        a.tap(a.target('Close scope', 1))
        assert a.page.evaluate('cuiGallery.exports.gallery_modal()') == 0
        assert a.target('Open focus trap', 1)['f'][7] == 1, 'Touch close did not restore opener focus'
    a.case('Focus Trap', 'touch opens scope, focuses child, closes and restores opener focus', focus_trap)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report', type=Path, default=Path('/tmp/cui-touch-followup-report.json'))
    parser.add_argument('--inventory-only', action='store_true')
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
            a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
            for name in FAMILIES:
                a.route(name)
                a.inventory[name] = a.snapshot()
            if not args.inventory_only:
                run_cases(a)
            assert not a.errors, a.errors
            args.report.write_text(json.dumps({'baseline': 'fda9f70', 'transport': 'Chromium CDP touch',
                'families': a.inventory, 'cases': a.results, 'page_errors': a.errors}, indent=2)+'\n')
            browser.close()
            failures = [r for r in a.results if r['status'] != 'verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} follow-up cases passed; report: {args.report}')
            if failures and not args.allow_defects:
                raise SystemExit(1)
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
