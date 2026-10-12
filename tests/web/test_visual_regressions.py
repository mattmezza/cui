#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Geometry regressions reported while using the real component gallery."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(**({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 900})
            page.goto(f'http://127.0.0.1:{server.server_port}/#Marker')
            def story(name):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>window.cuiGallery?.renderedComponent===name', arg=name)
            def text_commands():
                return page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_count()},(_,i)=>i)
                    .filter(i=>e.gallery_command(i,0)===2).map(i=>({
                    text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),
                    x:e.gallery_command(i,1),y:e.gallery_command(i,2),w:e.gallery_command(i,3),h:e.gallery_command(i,4)}));}''')
            story('Marker')
            text = text_commands()
            tasks = [c for c in text if c['text'] == '3 tasks']
            assert len(tasks) == 1, text
            label = next(c for c in text if c['text'] == 'Synchronizing workspace')
            assert tasks[0]['x'] >= label['x'] + label['w'] and abs(tasks[0]['y'] - label['y']) < 1, text
            for name in ('Pagination', 'Stepper'):
                story(name)
                semantics = page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_semantic_count()},(_,i)=>i)
                    .filter(i=>e.gallery_semantic(i,0)===1).map(i=>({
                    text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i))),
                    x:e.gallery_semantic(i,1),y:e.gallery_semantic(i,2),w:e.gallery_semantic(i,3),h:e.gallery_semantic(i,4)}));}''')
                matched = 0
                for text in text_commands():
                    buttons = [s for s in semantics if s['text'] == text['text']]
                    if not buttons:
                        continue
                    button = buttons[0]
                    assert abs(text['y'] + text['h']/2 - button['y'] - button['h']/2) < 1, (name, text, button)
                    matched += 1
                assert matched >= 2, name
            story('Rating')
            assert not any(c['text'] in ('★', '☆', '◐') for c in text_commands())
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>cuiGallery.exports.gallery_command(i,0)).filter(t=>t===5).length') == 8
            story('Image')
            assert page.evaluate('''()=>{const e=cuiGallery.exports,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===5);
                const pixel=new Uint8Array(e.memory.buffer,e.gallery_bitmap_pixels(i),e.gallery_bitmap(i,2)*e.gallery_bitmap(i,1));
                const colors=new Set();for(let j=0;j<pixel.length;j+=4)colors.add(`${pixel[j]},${pixel[j+1]},${pixel[j+2]}`);
                return colors.size>1000;}''')
            browser.close()
            print('PASS: Marker line, Pagination/Stepper text centering, graphical Rating, real Image pixels')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
