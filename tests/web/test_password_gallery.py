#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Exercise password modes in the actual gallery, including host IME and reveal."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            launch = {'headless': True}
            if args.chromium:
                launch['executable_path'] = args.chromium
            browser = p.chromium.launch(**launch)
            for width in (1280, 390):
                page = browser.new_page(viewport={'width': width, 'height': 900}, reduced_motion='reduce')
                errors = []
                page.on('pageerror', lambda e: errors.append(str(e)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Input')
                page.wait_for_function('window.cuiGallery?.renderedComponent==="Input"')
                session = page.context.new_cdp_session(page)
                def focus():
                    box = page.evaluate('''()=>{const e=cuiGallery.exports,r=document.querySelector('canvas').getBoundingClientRect();
                        for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===6&&!e.gallery_semantic(i,5))
                            return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}''')
                    page.mouse.click(box['x'], box['y'])
                    page.wait_for_function('document.activeElement.id==="cui-ime-input"')
                def length():
                    return page.evaluate('cuiGallery.exports.gallery_input_selection(2)')
                def drawn():
                    return page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>
                        new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))));}''')
                for mode, kind, purpose in ((4, 'password', 'current-password'), (5, 'text', 'current-password'), (6, 'password', 'new-password')):
                    page.locator('#input-mode').select_option(str(mode))
                    focus()
                    proxy = page.locator('#cui-ime-input')
                    assert proxy.get_attribute('type') == kind
                    assert proxy.get_attribute('autocomplete') == purpose
                    assert proxy.input_value() == ''
                    page.keyboard.press('Control+a')
                    session.send('Input.insertText', {'text': 's😀écret'})
                    page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===11')
                    strings = drawn()
                    if kind == 'password':
                        assert 's😀écret' not in ''.join(strings), strings
                        assert '•••••••' in ''.join(strings), strings
                    else:
                        assert 's😀écret' in ''.join(strings), strings
                    assert proxy.input_value() == ''
                    assert page.evaluate('''()=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_semantic_count();i++)
                        if(e.gallery_semantic(i,0)===6&&e.gallery_semantic(i,7))return e.gallery_semantic(i,8);}''') == 0
                    before = length()
                    session.send('Input.imeSetComposition', {'text': '😀é', 'selectionStart': 2, 'selectionEnd': 3})
                    page.wait_for_timeout(80)
                    assert length() == before
                    if kind == 'password':
                        assert '😀é' not in ''.join(drawn())
                    session.send('Input.insertText', {'text': '😀é'})
                    page.wait_for_function('(n)=>cuiGallery.exports.gallery_input_selection(2)===n+6', arg=before)
                    assert proxy.input_value() == ''
                    page.keyboard.press('Control+z')
                    page.wait_for_function('(n)=>cuiGallery.exports.gallery_input_selection(2)===n', arg=before)
                # Actual native preedit followed by the host's window-blur handler
                # must erase transient DOM text and finalized focus. Dispatching
                # the handler is evidence of host wiring, not physical OS focus.
                before = length()
                session.send('Input.imeSetComposition', {'text': 'SECRET', 'selectionStart': 6, 'selectionEnd': 6})
                page.wait_for_function('document.querySelector("#cui-ime-input").value==="SECRET"')
                page.evaluate('window.dispatchEvent(new Event("blur"))')
                page.wait_for_function('!document.querySelector("#cui-ime-input")&&cuiGallery.exports.gallery_text_config(0)===0')
                assert length() == before
                focus()
                session.send('Input.insertText', {'text': 'x'})
                page.wait_for_function('(n)=>cuiGallery.exports.gallery_input_selection(2)===n+1', arg=before)
                # A new finalized generation must replace secure proxy with an ordinary textarea.
                page.evaluate('location.hash="Textarea"')
                page.wait_for_function('cuiGallery.renderedComponent==="Textarea"')
                focus()
                assert page.locator('#cui-ime-input').evaluate('(node)=>node.tagName') == 'TEXTAREA'
                assert page.locator('#cui-ime-input').input_value() == ''
                assert not errors, errors
                page.close()
            browser.close()
            print('Actual gallery password/reveal/new-password/IME/Undo/ordinary textarea: desktop and 390px PASS')
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
