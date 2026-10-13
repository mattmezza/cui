#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual native text/IME, Undo and touch focus for selected Input format modes."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            for width in [1280, 390]:
                audit = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                audit.page.set_viewport_size({'width': width, 'height': 900})
                for mode, label, raw, formatted in [(7, 'Phone number', '4155550123', '(415) 555-0123'), (8, 'Amount', '12345.678', '12,345.67')]:
                    audit.route('Input')
                    audit.configure('input-mode', mode)
                    field = audit.target(label, 6)
                    point = audit.point(field, .25, .5)
                    audit.session.send('Input.dispatchTouchEvent', {'type': 'touchStart', 'touchPoints': [point]})
                    audit.session.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
                    audit.page.wait_for_function('document.activeElement.id === "cui-ime-input"')
                    previous = [text for text in audit.texts() if text]
                    audit.page.keyboard.press('Control+a')
                    audit.session.send('Input.insertText', {'text': raw})
                    audit.page.wait_for_function('(text)=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))).includes(text)}', arg=formatted)
                    assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(1)') == len(formatted)
                    audit.page.keyboard.press('Control+z')
                    audit.page.wait_for_timeout(80)
                    assert [text for text in audit.texts() if text] == previous, (mode, previous, audit.texts())
                    audit.page.keyboard.press('Control+a')
                    audit.session.send('Input.imeSetComposition', {'text': raw, 'selectionStart': len(raw), 'selectionEnd': len(raw)})
                    audit.page.wait_for_timeout(80)
                    audit.session.send('Input.insertText', {'text': raw})
                    audit.page.wait_for_timeout(80)
                    assert formatted in audit.texts()
                    assert not audit.errors, audit.errors
                # Disabling formatting must also clear its request workspace;
                # the checked engine rejects orphaned formatting resources.
                audit.configure('input-mode', 0)
                field = audit.target('Display name', 6)
                point = audit.point(field, .25, .5)
                audit.page.mouse.click(point['x'], point['y'])
                audit.page.wait_for_function('document.activeElement.id === "cui-ime-input"')
                audit.page.keyboard.press('Control+a')
                audit.session.send('Input.insertText', {'text': 'Ordinary42'})
                audit.page.wait_for_timeout(80)
                assert 'Ordinary42' in audit.texts()
                audit.page.close()
            browser.close()
    finally:
        server.shutdown()
    print('PASS: desktop/mobile Input pattern/number native text, IME commit, Undo, final caret and touch focus')


if __name__ == '__main__':
    main()
