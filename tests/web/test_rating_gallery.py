#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Exercise the live WASM rating with real pointer, keyboard and touch input."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading

from playwright.sync_api import expect, sync_playwright


RATING = """() => {
    const exports = cuiGallery.exports;
    const decoder = new TextDecoder();
    const canvas = document.getElementById('preview');
    const ctx = canvas.getContext('2d');
    const density = canvas.width / canvas.clientWidth;
    const slots = [];
    const paints = [];
    for (let i = 0; i < exports.gallery_semantic_count(); ++i) {
        const name = decoder.decode(new Uint8Array(
            exports.memory.buffer, exports.gallery_semantic_name(i), exports.gallery_semantic_length(i)));
        if (/^[1-5] of 5$/.test(name))
            slots[Number(name[0])-1] = [1, 2, 3, 4].map(field => exports.gallery_semantic(i, field));
    }
    for (let i = 0; i < exports.gallery_count(); ++i) {
        if (exports.gallery_command(i, 0) !== 5) continue;
        const [x, y, width, height] = [1, 2, 3, 4].map(field => exports.gallery_command(i, field));
        const tint = [10, 11, 12, 13].map(field => exports.gallery_bitmap(i, field));
        const revision = [0, 1].map(field => exports.gallery_bitmap_revision(i, field)).join(':');
        const pixel = [...ctx.getImageData(
            Math.floor((x + width / 2) * density), Math.floor((y + height / 2) * density), 1, 1).data];
        paints.push({height, width, revision, tint, pixel});
    }
    return {slots, paints};
}"""


def wait_after(page, action):
    before = page.evaluate('cuiGallery.frame')
    action()
    page.wait_for_function('previous => cuiGallery.frame > previous', arg=before)


def probe(page):
    return page.evaluate(RATING)


def controls(page):
    page.emulate_media(color_scheme='light')
    page.goto('/#Rating')
    page.wait_for_function('window.cuiGallery?.renderedComponent === "Rating"')
    canvas = page.locator('#preview')
    canvas.scroll_into_view_if_needed()
    slider = page.locator('#accessible').get_by_role('slider', name='Rate your experience')
    expect(slider).to_have_attribute('aria-valuenow', '3')
    return canvas, slider


def point(page, canvas, slot, fraction=.5):
    left, top, width, height = probe(page)['slots'][slot - 1]
    bounds = canvas.bounding_box()
    assert left >= 0 and left + width <= bounds['width'] + 1
    return bounds['x'] + left + width * fraction, bounds['y'] + top + height / 2


def desktop(page):
    canvas, slider = controls(page)
    initial = probe(page)['paints']
    assert len(initial) == 8, initial
    initial_tints = {tuple(mark['tint']) for mark in initial}
    assert len(initial_tints) == 2
    wait_after(page, lambda: page.mouse.move(*point(page, canvas, 5)))
    hovered = probe(page)['paints']
    assert len(hovered) == 8
    assert len({tuple(mark['tint']) for mark in hovered}) == 1
    expect(slider).to_have_attribute('aria-valuenow', '3')
    wait_after(page, lambda: page.mouse.move(0, 0))
    assert probe(page)['paints'] == initial

    canvas.focus()
    page.keyboard.press('Tab')
    page.keyboard.press('ArrowRight')
    expect(slider).to_have_attribute('aria-valuenow', '4')
    wait_after(page, lambda: page.mouse.click(*point(page, canvas, 3)))
    expect(slider).to_have_attribute('aria-valuenow', '2')
    assert len(probe(page)['paints']) == 7
    wait_after(page, lambda: page.locator('#rating-half').check())
    wait_after(page, lambda: page.mouse.click(*point(page, canvas, 4, .25)))
    expect(slider).to_have_attribute('aria-valuenow', '3.5')
    assert any(mark['width'] < mark['height'] * .55 for mark in probe(page)['paints'])
    page.keyboard.press('ArrowRight')
    expect(slider).to_have_attribute('aria-valuenow', '4')

    stars = {mark['revision'] for mark in probe(page)['paints']}
    wait_after(page, lambda: page.locator('#rating-shape').select_option('1'))
    hearts = probe(page)['paints']
    assert not stars & {mark['revision'] for mark in hearts}
    assert any(mark['pixel'] == [190, 49, 85, 255] for mark in hearts), hearts
    expect(page.locator('#snippet')).to_contain_text('prepare_heart_rating')
    wait_after(page, lambda: page.locator('#design-size').select_option('5'))
    assert max(mark['height'] for mark in probe(page)['paints']) > max(mark['height'] for mark in hearts)


def touch(page):
    canvas, slider = controls(page)
    wait_after(page, lambda: page.locator('#design-size').select_option('14'))
    wait_after(page, lambda: page.touchscreen.tap(*point(page, canvas, 4)))
    expect(slider).to_have_attribute('aria-valuenow', '4')
    assert len({tuple(mark['tint']) for mark in probe(page)['paints']}) == 2
    wait_after(page, lambda: page.locator('#disabled').check())
    expect(slider).to_be_disabled()
    wait_after(page, lambda: page.touchscreen.tap(*point(page, canvas, 5)))
    expect(slider).to_have_attribute('aria-valuenow', '4')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', default='build/web', type=Path)
    args = parser.parse_args()
    assert (args.directory / 'gallery.wasm').is_file(), args.directory
    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                 partial(SimpleHTTPRequestHandler, directory=args.directory))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True)
            options = (('desktop preview, clear, half steps and custom masks', desktop,
                        {'viewport': {'width': 1240, 'height': 900}}),
                       ('390px touch layout, selection and disabled state', touch,
                        {'viewport': {'width': 390, 'height': 840}, 'has_touch': True,
                         'device_scale_factor': 2, 'is_mobile': True}))
            for name, check, settings in options:
                page = browser.new_page(base_url=f'http://127.0.0.1:{server.server_port}', **settings)
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                try:
                    check(page)
                    assert not errors, errors
                    print(f'VERIFIED: Rating {name}', flush=True)
                finally:
                    page.close()
            browser.close()
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == '__main__':
    main()
