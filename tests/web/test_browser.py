#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Chromium tests of the CUI WASM gallery, including repo-relative hosting."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium', help='Optional installed Chromium executable')
    args = parser.parse_args()
    root = args.directory.resolve()
    # Mount as /cui/ to exercise the same URL shape as GitHub project Pages.
    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            if path.startswith('/cui/'):
                path = path[4:]
            return super().translate_path(path)
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(root)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, device_scale_factor=2)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/')
            page.wait_for_function('window.cuiGallery && window.cuiGallery.exports.gallery_count() > 0')
            assert page.locator('#catalog a').count() == 78
            assert page.locator('#name').inner_text() == 'Button'
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 0
            canvas = page.locator('canvas')
            box = canvas.bounding_box()
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function('cuiGallery.exports.gallery_clicks() === 1')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.exports.gallery_clicks() === 2')
            page.locator('#disabled').check()
            page.wait_for_function('cuiGallery.exports.gallery_semantic(0,5) === 1')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_timeout(80)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 2
            page.locator('#theme').select_option('1')
            assert page.locator('body.dark').count() == 1
            for name in page.evaluate('Object.keys(cuiGallery.stories)'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name', arg=name)
                page.wait_for_timeout(50)
                assert page.locator('#preview-card').is_visible()
                assert 'draw commands' in page.locator('#preview-note').inner_text()
            page.evaluate('location.hash="Checkbox"')
            page.wait_for_function('document.querySelector("#name").textContent==="Checkbox"')
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.press('Space')
            page.wait_for_function('cuiGallery.exports.gallery_semantic(0,6) === 1')
            page.evaluate('location.hash="Slider"')
            page.wait_for_function('document.querySelector("#name").textContent==="Slider"')
            page.wait_for_timeout(60)
            canvas.focus()
            page.keyboard.press('Tab')
            initial = page.evaluate('cuiGallery.exports.gallery_semantic(0,8)')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(initial)=>cuiGallery.exports.gallery_semantic(0,8)>initial', arg=initial)
            page.evaluate('location.hash="Input"')
            page.wait_for_function('document.querySelector("#name").textContent==="Input"')
            page.wait_for_timeout(60)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.type('Z')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));}).some(s=>s.includes("Z"))')
            page.evaluate('location.hash="Dock"')
            page.wait_for_function('document.querySelector("#name").textContent==="Dock"')
            assert page.locator('#pending').is_visible()
            assert not page.locator('#preview-card').is_visible()
            page.locator('#search').fill('button')
            assert 0 < page.locator('#catalog a').count() < 78
            page.locator('#search').fill('')
            page.set_viewport_size({'width': 390, 'height': 844})
            page.locator('#menu').click()
            assert page.locator('#sidebar').is_visible()
            page.locator('#catalog a', has_text='Button').first.click()
            page.wait_for_function('document.querySelector("#name").textContent==="Button"')
            page.wait_for_timeout(100)
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
            page.screenshot(path=str(root / 'browser-mobile.png'))
            assert not errors, errors
            browser.close()
            print('PASS: 78 catalog entries, 22 real WASM stories, pointer/keyboard/disabled controls, slider, input, themes, pending status, search, 390px layout, /cui/ hosting')
    finally:
        server.shutdown()
        server.server_close()

if __name__ == '__main__':
    main()
