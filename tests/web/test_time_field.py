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

            def click(name, role):
                point = page.evaluate('''([name,role])=>{
                    const e=cuiGallery.exports,d=new TextDecoder();
                    const c=document.querySelector('canvas').getBoundingClientRect();
                    for(let i=0;i<e.gallery_semantic_count();i++)
                        if(e.gallery_semantic(i,0)===role&&d.decode(new Uint8Array(
                            e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                            return [c.x+e.gallery_semantic(i,1)+e.gallery_semantic(i,3)/2,
                                    c.y+e.gallery_semantic(i,2)+e.gallery_semantic(i,4)/2];
                    throw Error('Missing semantic '+name);
                }''', [name, role])
                page.mouse.click(*point)
                page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')

            def stats(field):
                return page.evaluate('field=>cuiGallery.exports.gallery_control_stats(field)', field)

            def textbox(name, attribute=None, value=None):
                page.wait_for_function('''([name,attribute,value])=>{
                    const node=Array.from(document.querySelectorAll('#accessible [role=textbox]'))
                        .find(node=>node.getAttribute('aria-label')===name);
                    return node&&(!attribute||node.getAttribute(attribute)===value);
                }''', arg=[name, attribute, value])

            textbox('Hour')
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
            assert not errors, errors
            browser.close()
            print('PASS: TimeField keyboard change, H12/Arabic locale, read-only, rejected bounds, validation reset')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
