#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Focused real-WASM localized TimeField checks, independent of other stories."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread

from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            return super().translate_path(path[4:] if path.startswith('/cui/') else path)

        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                partial(Handler, directory=str(args.directory.resolve())))
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = playwright.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, device_scale_factor=2)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            page.locator('canvas').scroll_into_view_if_needed()
            assert 'CUI_TIME_FIELD_IME_V1' in page.locator('#snippet').text_content()

            def box(name, role):
                return page.evaluate('''([name,role])=>{
                    const e=cuiGallery.exports,d=new TextDecoder();
                    for(let i=0;i<e.gallery_semantic_count();i++)
                        if(e.gallery_semantic(i,0)===role&&d.decode(new Uint8Array(
                            e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                            return [1,2,3,4].map(field=>e.gallery_semantic(i,field));
                    throw Error('Missing semantic '+name);
                }''', [name, role])

            def next_paint():
                page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')

            def click(name, role):
                x, y, w, h = box(name, role)
                canvas = page.locator('canvas').bounding_box()
                page.mouse.click(canvas['x'] + x + w / 2, canvas['y'] + y + h / 2)
                next_paint()

            def stats(field):
                return page.evaluate('field=>cuiGallery.exports.gallery_control_stats(field)', field)

            def textbox(name, attribute=None, value=None):
                page.wait_for_function('''([name,attribute,value])=>{
                    const node=Array.from(document.querySelectorAll('#accessible [role=textbox]'))
                        .find(node=>node.getAttribute('aria-label')===name);
                    return node&&(!attribute||node.getAttribute(attribute)===value);
                }''', arg=[name, attribute, value])

            textbox('Hour')
            size = page.locator('#design-size')
            assert size.is_visible() and size.input_value() == '3'
            assert box('Time', 9)[3] == 32
            for value, expected, name in [
                ('1', 20, 'XS'), ('9', 80, '4XL'), ('14', 264, '9XL'), ('3', 32, 'BASE')
            ]:
                size.select_option(value)
                next_paint()
                root = box('Time', 9)
                cell = box('Hour', 6)
                assert root[3] == expected, (value, root)
                assert abs(root[1] + root[3] / 2 - cell[1] - cell[3] / 2) < 1
                assert root[0] + root[2] < page.locator('canvas').bounding_box()['width']
                assert f'CUI_DESIGN_{name}' in page.locator('#snippet').text_content()
            density = page.locator('#density')
            density.select_option('1')
            next_paint()
            assert box('Time', 9)[3] == 28
            density.select_option('0')
            next_paint()
            click('Hour', 6)
            page.keyboard.press('ArrowUp')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(16)===103000')
            callbacks = stats(17)
            assert callbacks == 1
            click('12-hour', 2)
            textbox('AM/PM')
            click('Arabic', 2)
            textbox('ساعة')
            click('Read only', 2)
            textbox('ساعة', 'aria-readonly', 'true')
            click('ساعة', 6)
            page.keyboard.press('ArrowUp')
            page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')
            assert stats(16) == 103000 and stats(17) == callbacks
            click('Read only', 2)
            click('12-hour', 2)
            click('Bounds', 2)
            click('ساعة', 6)
            page.keyboard.press('Home')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===5')
            assert stats(16) == 103000 and stats(17) == callbacks
            click('Clear validation', 1)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===0')
            click('دقيقة', 6)
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            cdp = page.context.new_cdp_session(page)

            def candidate(value):
                cdp.send('Input.imeSetComposition',
                         {'text': value, 'selectionStart': len(value), 'selectionEnd': len(value)})

            def painted(value, present=True):
                page.wait_for_function('''([value,present])=>{
                    const e=cuiGallery.exports, decoder=new TextDecoder();
                    const found=Array.from({length:e.gallery_count()},(_,i)=>decoder.decode(
                        new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
                                       e.gallery_command_length(i)))).includes(value);
                    return found===present;
                }''', arg=[value, present])

            candidate('٢٠')
            painted('٢٠')
            assert stats(16) == 103000 and stats(17) == callbacks
            cdp.send('Input.insertText', {'text': '٢٠'})
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(16)===102000')
            assert stats(17) == callbacks + 1

            candidate('٦٠')
            painted('٦٠')
            cdp.send('Input.insertText', {'text': '٦٠'})
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===4')
            assert stats(16) == 102000 and stats(17) == callbacks + 1

            candidate('٤٢')
            painted('٤٢')
            candidate('')
            painted('٤٢', present=False)
            assert stats(16) == 102000 and stats(17) == callbacks + 1
            assert not errors, errors
            page.close()

            page = browser.new_page(viewport={'width': 390, 'height': 880}, device_scale_factor=2,
                                    is_mobile=True, has_touch=True)
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            stage = page.locator('#video-player-stage')
            stage.scroll_into_view_if_needed()
            x, y, w, h = box('Hour', 6)
            canvas = page.locator('canvas').bounding_box()
            page.touchscreen.tap(canvas['x'] + x + w / 2, canvas['y'] + y + h / 2)
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            page.locator('#design-size').select_option('14')
            next_paint()
            stage.scroll_into_view_if_needed()
            rectangle = stage.bounding_box()
            x, y = round(rectangle['x'] + rectangle['width'] - 35), round(rectangle['y'] + 120)
            cdp = page.context.new_cdp_session(page)
            cdp.send('Input.dispatchTouchEvent', {'type': 'touchStart',
                                                  'touchPoints': [{'x': x, 'y': y}]})
            for offset in range(25, 226, 25):
                cdp.send('Input.dispatchTouchEvent', {
                    'type': 'touchMove', 'touchPoints': [{'x': x - offset, 'y': y}]})
            cdp.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            page.wait_for_function('document.getElementById("video-player-stage").scrollLeft > 80')
            assert not errors, errors
            browser.close()
            print('PASS: TimeField sizes, density, touch, keyboard, H12/Arabic locale, and IME')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
