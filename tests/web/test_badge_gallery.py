#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Verify the composed Badge inbox through actual WASM and Canvas rendering."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading

from playwright.sync_api import sync_playwright
from test_questionnaire_progress_gallery import compile_usage
from test_touch_components import SNAPSHOT


TARGET = f"""(query) => ({SNAPSHOT})().find(
    row => row.name === query.name && row.f[0] === query.role)"""


def semantic(page, name, role=19):
    return page.evaluate(TARGET, {'name': name, 'role': role})


def wait_for(page, name, role=19, field=None, value=None, absent=False):
    return page.wait_for_function(f"""query => {{
        const item = ({TARGET})(query);
        return query.absent ? !item : item && (query.field === null ||
            item.f[query.field] === query.value);
    }}""", arg={'name': name, 'role': role, 'field': field, 'value': value, 'absent': absent})


def center(page, child, marker=None):
    canvas = page.locator('#preview')
    canvas.scroll_into_view_if_needed()
    rect = canvas.bounding_box()
    _, x, y, w, h, *_ = semantic(page, child, 1)['f']
    left, right, top, bottom = x, x+w, y, y+h
    if marker:
        _, x, y, w, h, *_ = semantic(page, marker)['f']
        left, right = max(left, x), min(right, x+w)
        top, bottom = max(top, y), min(bottom, y+h)
    assert left < right and top < bottom, (child, marker)
    return rect['x']+(left+right)/2, rect['y']+(top+bottom)/2


def verify_photos(page):
    colors = page.evaluate(f"""names => {{
        const canvas = document.querySelector('#preview'), px = canvas.width / canvas.clientWidth;
        const ctx = canvas.getContext('2d');
        return names.map(name => {{
            const [,x,y,w,h] = ({TARGET})({{name,role:1}}).f;
            const pixels = ctx.getImageData((x + w/2 - 4)*px, (y + h/2 - 4)*px, 8*px, 8*px).data;
            const colors = new Set();
            for (let i=0; i<pixels.length; i+=4)
                colors.add(pixels.slice(i,i+3).join(','));
            return [colors.size, [...colors][0]];
        }});
    }}""", ['Alex Morgan', 'Jamie Chen', 'Robin Hayes'])
    assert all(count > 8 for count, _ in colors), colors
    assert len({color for _, color in colors}) == 3, colors


def verify_recipe(page, touch):
    errors = []
    page.on('pageerror', lambda error: errors.append(str(error)))
    page.set_default_timeout(8000)
    wait_for(page, 'Alex Morgan is verified')
    wait_for(page, 'Escalations: 125 unread')
    assert page.evaluate("""() => {
        const e = cuiGallery.exports, decode = new TextDecoder();
        return Array.from({length:e.gallery_count()}, (_,i) => decode.decode(new Uint8Array(
            e.memory.buffer, e.gallery_command_text(i), e.gallery_command_length(i)))).includes('99+');
    }""")
    assert page.locator('#design-size').is_hidden()
    verify_photos(page)
    for mark in ('Alex Morgan is verified', 'Jamie Chen is online', 'Robin Hayes is away'):
        status = semantic(page, mark)
        assert status and not status['f'][33] and not status['f'][34], status

    hit = page.touchscreen.tap if touch else page.mouse.click
    initial = semantic(page, 'Alex Morgan', 1)['f'][1:5]
    hit(*center(page, 'Alex Morgan', 'Alex Morgan is verified'))
    wait_for(page, 'Alex Morgan opened')
    wait_for(page, 'Alex Morgan: 5 unread', absent=True)
    wait_for(page, 'Alex Morgan is verified')
    assert semantic(page, 'Alex Morgan', 1)['f'][1:5] == initial
    if touch:
        hit(*center(page, 'Jamie Chen', 'Jamie Chen is online'))
    else:
        wait_for(page, 'Alex Morgan', 1, 7, 1)
        page.keyboard.press('Tab')
        wait_for(page, 'Jamie Chen', 1, 7, 1)
        page.keyboard.press('Enter')
    wait_for(page, 'Jamie Chen opened')

    hit(*center(page, 'Open inbox', 'Inbox: 7 unread'))
    wait_for(page, 'Inbox opened')
    wait_for(page, 'Inbox: 7 unread', absent=True)
    hit(*center(page, 'Restore notifications'))
    wait_for(page, 'Inbox: 7 unread')
    wait_for(page, 'Alex Morgan: 5 unread')
    wait_for(page, 'Select an inbox or team member')

    page.locator('#disabled').check()
    wait_for(page, 'Alex Morgan', 1, 5, 1)
    before = page.evaluate('cuiGallery.frame')
    hit(*center(page, 'Alex Morgan', 'Alex Morgan is verified'))
    page.wait_for_function('before => cuiGallery.frame > before', arg=before)
    wait_for(page, 'Alex Morgan: 5 unread')
    assert not semantic(page, 'Alex Morgan opened')
    page.locator('#disabled').uncheck()
    wait_for(page, 'Alex Morgan', 1, 5, 0)

    if not touch:
        page.set_viewport_size({'width': 252, 'height': 950})
        page.wait_for_function(f"""() => ({TARGET})({{name:'Alex Morgan',role:1}}).f[2] <
            ({TARGET})({{name:'Robin Hayes',role:1}}).f[2]""")
        assert semantic(page, 'Alex Morgan', 1)['f'][1] == semantic(page, 'Robin Hayes', 1)['f'][1]
        page.set_viewport_size({'width': 1280, 'height': 950})
        page.wait_for_function(f"""() => ({TARGET})({{name:'Alex Morgan',role:1}}).f[2] ===
            ({TARGET})({{name:'Robin Hayes',role:1}}).f[2]""")
        page.evaluate("""() => Object.defineProperty(navigator, 'clipboard', {
            configurable: true, value: {writeText: async code => window.copiedBadge = code}})""")
        page.locator('#copy-snippet').click()
        page.wait_for_function('window.copiedBadge?.includes("badge_inbox_draw")')
        code = page.evaluate('window.copiedBadge')
        root = Path(__file__).resolve().parents[2]
        assert code == (root / 'examples/badge_recipes/inbox.h').read_text()
        compile_usage(code)

    layout = page.get_by_role('combobox', name='Badge example')
    layout.select_option('1')
    wait_for(page, 'Configured artwork')
    assert page.locator('#design-size').is_visible()
    for name in ('99+', 'Notification', 'Status'):
        wait_for(page, name)
    if not touch:
        for size, extent in (('3', 16), ('2', 14), ('5', 18), ('1', 12)):
            page.locator('#design-size').select_option(size)
            wait_for(page, 'Status', field=3, value=extent)
    wait_for(page, '7')
    hit(*center(page, 'Inbox'))
    wait_for(page, '8')
    layout.select_option('0')
    wait_for(page, 'Inbox: 7 unread')
    assert page.evaluate('cuiGallery.exports.gallery_error()') == 0
    assert not errors, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                 partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as app:
            browser = app.chromium.launch(headless=True, executable_path=args.chromium)
            for width, dpr, touch in ((1280, 1, False), (390, 2, True)):
                context = browser.new_context(viewport={'width': width, 'height': 950},
                                              device_scale_factor=dpr, has_touch=touch,
                                              color_scheme='dark' if touch else 'light')
                page = context.new_page()
                page.goto(f'http://127.0.0.1:{server.server_port}/#Badge')
                page.wait_for_function('window.cuiGallery?.renderedComponent === "Badge"')
                verify_recipe(page, touch)
                context.close()
            browser.close()
        print('PASS Badge inbox photos, passive nested markers, mouse, keyboard, touch, resize and copy')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
