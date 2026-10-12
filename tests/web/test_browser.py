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
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, device_scale_factor=2, permissions=['clipboard-read', 'clipboard-write'])
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/')
            page.wait_for_function('window.cuiGallery && window.cuiGallery.exports.gallery_count() > 0')
            assert page.request.get(f'http://127.0.0.1:{server.server_port}/cui/LICENSE.txt').ok
            assert page.locator('#catalog a').count() == 78
            assert page.locator('#name').inner_text() == 'Button'
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 0
            assert page.locator('#accessible [aria-busy=true]').count() == 1
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
            page.evaluate('location.hash="Skeleton"')
            page.wait_for_function('document.querySelector("#name").textContent==="Skeleton"')
            page.wait_for_timeout(80)
            signature = 'JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))'
            initial_signature = page.evaluate(signature)
            page.wait_for_timeout(200)
            assert page.evaluate(signature) != initial_signature
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.exports.gallery_redraw()===0')
            initial_signature = page.evaluate(signature)
            page.wait_for_timeout(100)
            assert page.evaluate(signature) == initial_signature
            page.emulate_media(reduced_motion='no-preference')
            page.wait_for_function('cuiGallery.exports.gallery_redraw()===1')
            page.evaluate('location.hash="Clipboard"')
            page.wait_for_function('document.querySelector("#name").textContent==="Clipboard"')
            page.wait_for_timeout(80)
            box = canvas.bounding_box()
            for index, expected in [(0, 'Copied: Hello from CUI'), (1, 'Read: Hello from CUI')]:
                bounds = page.evaluate('(index)=>Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(index,i+1))', index)
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
                page.wait_for_function('(expected)=>' + texts + '.some(s=>s.includes(expected))', arg=expected)
            page.evaluate('navigator.clipboard.writeText("é".repeat(100))')
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(1,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function(texts + '.some(s=>s.includes("Read: é"))')
            assert page.evaluate(texts + '.every(s=>!s.includes("�"))')
            page.evaluate('location.hash="Focus%20Trap"')
            page.wait_for_function('document.querySelector("#name").textContent==="Focus Trap"')
            page.wait_for_timeout(80)
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            initial_focus = page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,7))')
            page.keyboard.press('Tab')
            page.keyboard.press('Tab')
            page.wait_for_timeout(60)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,7))') == initial_focus
            assert page.evaluate('document.activeElement.id') == 'preview'
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
            for name, initial, key, expected in [('NumberInput', 42, 'ArrowUp', 43), ('Rating', 3, 'ArrowRight', 3.5)]:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name', arg=name)
                page.wait_for_timeout(80)
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,8)') == initial
                canvas.focus()
                page.keyboard.press('Tab')
                page.keyboard.press(key)
                page.wait_for_function('(expected)=>cuiGallery.exports.gallery_semantic(0,8)===expected', arg=expected)
            page.evaluate('location.hash="Combobox"')
            page.wait_for_function('document.querySelector("#name").textContent==="Combobox"')
            page.wait_for_timeout(80)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.type('Ban')
            page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(texts + '.some(s=>s.includes("1 matching options"))')
            page.keyboard.press('ArrowDown')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
            for name in ['Select', 'Menu']:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name', arg=name)
                page.wait_for_timeout(80)
                box = canvas.bounding_box()
                bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
                page.keyboard.press('Enter')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
                page.keyboard.press('Tab')
                page.wait_for_timeout(50)
                assert page.evaluate('document.activeElement.id') == 'preview'
                page.keyboard.press('ArrowDown')
                page.keyboard.press('Enter')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
            page.evaluate('location.hash="Tabs"')
            page.wait_for_function('document.querySelector("#name").textContent==="Tabs"')
            page.wait_for_timeout(80)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>{const e=cuiGallery.exports;return e.gallery_semantic(i,0)===8 && e.gallery_semantic(i,11) && new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==="Activity";}).some(Boolean)')
            for name, expected in [('Dock', '2 tab regions'), ('Calendar', '2026-09-12'), ('Plot', '16.67')]:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name', arg=name)
                page.wait_for_timeout(80)
                box = canvas.bounding_box()
                bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
                page.wait_for_function('(expected)=>' + texts + '.some(s=>s.includes(expected))', arg=expected)
                assert 'base model' in page.locator('#status').inner_text()
            page.evaluate('location.hash="Tooltip"')
            page.wait_for_function('document.querySelector("#name").textContent==="Tooltip"')
            page.wait_for_timeout(80)
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            box = canvas.bounding_box()
            page.mouse.move(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(texts + '.some(s=>s.includes("floating tooltip"))')
            canvas.focus()
            page.keyboard.press('Escape')
            page.wait_for_function(texts + '.every(s=>!s.includes("floating tooltip"))')
            page.evaluate('location.hash="HoverCard"')
            page.wait_for_function('document.querySelector("#name").textContent==="HoverCard"')
            page.wait_for_timeout(80)
            page.mouse.move(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function(texts + '.some(s=>s.includes("Alex Morgan"))')
            page.mouse.move(box['x'] + box['width'] - 8, box['y'] + box['height'] - 8)
            page.locator('#theme').focus()
            page.wait_for_function(texts + '.every(s=>!s.includes("Alex Morgan"))')
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
            page.evaluate('location.hash="Chart"')
            page.wait_for_function('document.querySelector("#name").textContent==="Chart"')
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
            overflow = []
            for name in page.evaluate('Object.keys(cuiGallery.stories)'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name', arg=name)
                page.wait_for_timeout(40)
                bad = page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{const e=cuiGallery.exports;return [e.gallery_command(i,0),e.gallery_command(i,1),e.gallery_command(i,2),e.gallery_command(i,3),e.gallery_command(i,4)];}).filter(c=>c[0]===2&&(c[1]<0||c[2]<0||c[1]+c[3]>document.querySelector("canvas").clientWidth+1||c[2]+c[4]>261))')
                if bad:
                    overflow.append((name, bad))
            assert not overflow, overflow
            page.evaluate('location.hash="Button"')
            page.wait_for_function('document.querySelector("#name").textContent==="Button"')
            page.wait_for_timeout(50)
            page.screenshot(path=str(root / 'browser-mobile.png'))
            page.evaluate('location.hash="%"')
            page.wait_for_timeout(60)
            assert page.locator('#name').inner_text() == 'Button'
            assert not errors, errors
            browser.close()
            print('PASS: 78 catalog entries, 39 real WASM stories, pointer/keyboard/disabled controls, slider, input, themes, pending status, search, 390px layout, /cui/ hosting')
    finally:
        server.shutdown()
        server.server_close()

if __name__ == '__main__':
    main()
