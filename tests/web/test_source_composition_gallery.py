#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM Label privacy and composed Empty native actions at two widths."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit



def assert_empty_text_fits(a):
    bounds = a.page.evaluate("""()=>{
        const e=cuiGallery.exports, canvas=document.querySelector('canvas');
        const decoder=new TextDecoder();
        const text=Array.from({length:e.gallery_count()},(_,i)=>i)
            .filter(i=>e.gallery_command(i,0)===2 && e.gallery_command(i,8)>0)
            .map(i=>({text:decoder.decode(new Uint8Array(e.memory.buffer,
                e.gallery_command_text(i),e.gallery_command_length(i))),
                x:e.gallery_command(i,1),y:e.gallery_command(i,2),
                width:e.gallery_command(i,3),height:e.gallery_command(i,4)}));
        return {width:canvas.clientWidth,height:canvas.clientHeight,text};
    }""")
    assert bounds['text'], bounds
    assert any(t['text'].startswith('Projects created:') for t in bounds['text']), bounds
    for text in bounds['text']:
        assert text['x'] >= 0 and text['y'] >= 0, text
        assert text['x'] + text['width'] <= bounds['width'] + 1, (text, bounds)
        assert text['y'] + text['height'] <= bounds['height'] + 1, (text, bounds)

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
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            for width in (1280, 390):
                a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                a.page.set_viewport_size({'width': width, 'height': 900})
                a.route('Label')
                text = ''.join(a.texts())
                names = [s['name'] for s in a.snapshot()]
                assert 'Workspace Ready to collaborate' in text
                assert 'CAFÉ and café' in text
                assert 'Private workspace' not in text and 'Private workspace' not in names
                assert any(name and set(name) == {'•'} for name in names), names
                assert a.page.locator('#scrollbar-policy').is_hidden()
                assert 'cui_label_source' in a.page.locator('#snippet').text_content()
                a.route('Empty')
                assert 'No projects yet' in ''.join(a.texts())
                assert 'Projects created: 0' in ''.join(a.texts())
                assert_empty_text_fits(a)
                button = a.target('Create project', 1)
                assert button['f'][3] > 0 and button['f'][4] > 0
                a.tap(button)
                a.page.wait_for_function("()=>document.querySelector('#preview-note').textContent.length>0")
                assert 'Projects created: 1' in ''.join(a.texts())
                assert_empty_text_fits(a)
                a.page.keyboard.press('Enter')
                a.page.wait_for_timeout(80)
                assert 'Projects created: 2' in ''.join(a.texts())
                assert_empty_text_fits(a)
                a.configure('disabled', True)
                a.tap(a.target('Create project', 1))
                a.page.keyboard.press('Enter')
                a.page.wait_for_timeout(80)
                assert 'Projects created: 2' in ''.join(a.texts())
                assert_empty_text_fits(a)
                assert 'cui_empty_ex' in a.page.locator('#snippet').text_content()
                assert a.page.locator('#scrollbar-policy').is_hidden()
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS: Label privacy and Empty pointer/keyboard/disabled actions at {width}px', flush=True)
            browser.close()
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
