#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Exercise real HoverCard layout, simulated window host and Toolbar Find input."""
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
            page.goto(f'http://127.0.0.1:{server.server_port}/#HoverCard')

            def story(name):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>window.cuiGallery?.renderedComponent===name', arg=name)

            def semantics():
                return page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_semantic_count()},(_,i)=>({i,
                        name:d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i))),
                        role:e.gallery_semantic(i,0),disabled:e.gallery_semantic(i,5),
                        b:[1,2,3,4].map(f=>e.gallery_semantic(i,f))}));}''')

            def semantic(name):
                return next(s for s in semantics() if s['name'] == name)

            def click(name):
                b = semantic(name)['b']
                canvas = page.locator('canvas').bounding_box()
                page.mouse.click(canvas['x'] + b[0] + b[2]/2, canvas['y'] + b[1] + b[3]/2)

            def text():
                return page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===2)
                    .map(i=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))).join(String.fromCharCode(10));}''')

            def wait_text(value):
                page.wait_for_function('''(value)=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===2)
                    .some(i=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))).includes(value));}''', arg=value)

            def wait_disabled(name, disabled):
                page.wait_for_function('''({name,disabled})=>{const e=cuiGallery.exports,d=new TextDecoder();
                    for(let i=0;i<e.gallery_semantic_count();i++)if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                        return Boolean(e.gallery_semantic(i,5))===disabled;return false;}''', arg={'name': name, 'disabled': disabled})

            story('HoverCard')
            click('Hover or focus here')
            page.wait_for_function('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                for(let i=0;i<e.gallery_semantic_count();i++)if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==='Profile details')return e.gallery_semantic(i,4)>30&&e.gallery_semantic(i,4)<100;return false;}''')
            assert 'Alex Morgan' in text() and 'A composable floating card.' in text()
            assert 30 < semantic('Profile details')['b'][3] < 100

            story('TitleBar')
            page.locator('#title-decoration').select_option('3')
            page.wait_for_function('document.querySelectorAll("#accessible [role=button]").length===3')
            assert 'Browser simulation of OS traffic lights' in text()
            assert len([s for s in semantics() if s['name'].startswith('Simulated ')]) == 3
            assert page.evaluate('''()=>{const e=cuiGallery.exports,c=document.querySelector('canvas'),ctx=c.getContext('2d');
                const colors=[[255,95,87],[254,188,46],[40,200,64]];
                return colors.every(color=>{for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===0&&
                    color.every((v,j)=>e.gallery_command(i,j+5)===v)&&e.gallery_command(i,3)===12){
                    const x=(e.gallery_command(i,1)+6)*c.width/c.clientWidth,y=(e.gallery_command(i,2)+6)*c.height/c.clientHeight;
                    const pixel=ctx.getImageData(Math.floor(x),Math.floor(y),1,1).data;
                    return color.every((v,j)=>Math.abs(pixel[j]-v)<=1);}return false;});}''')
            before = page.evaluate('cuiGallery.exports.gallery_title_action(1)')
            click('Simulated minimize')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(1)===n+1', arg=before)
            click('Simulated maximize')
            page.wait_for_function('cuiGallery.exports.gallery_title_action(2)>0')
            assert 'Preview: maximized' in text()
            wait_disabled('Simulated restore', False)
            click('Simulated restore')
            page.wait_for_function('cuiGallery.exports.gallery_title_action(3)>0')
            assert 'Preview: normal' in text()
            before = page.evaluate('cuiGallery.exports.gallery_title_action(4)')
            click('Simulated close')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(4)===n+1', arg=before)
            page.locator('#title-close').check()
            before = page.evaluate('cuiGallery.exports.gallery_title_action(7)')
            click('Simulated close')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(7)===n+1', arg=before)
            page.locator('#disabled').check()
            wait_disabled('Simulated minimize', True)
            before = page.evaluate('cuiGallery.exports.gallery_title_action(1)')
            assert semantic('Simulated minimize')['disabled']
            click('Simulated minimize')
            assert page.evaluate('cuiGallery.exports.gallery_title_action(1)') == before
            page.locator('#disabled').uncheck()
            wait_disabled('Simulated minimize', False)
            page.locator('#title-fullscreen').check()
            page.wait_for_function('document.querySelectorAll("#accessible [role=button]").length===0')
            assert 'native controls hidden in fullscreen' in text()

            story('Toolbar')
            assert 'Find: 2 matching lines' in text()
            page.locator('canvas').focus()
            page.keyboard.press('Control+f')
            page.wait_for_function('''()=>{const e=cuiGallery.exports,d=new TextDecoder();for(let i=0;i<e.gallery_semantic_count();i++)
                if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==='Find')return e.gallery_semantic(i,7)===1;return false;}''')
            before = page.evaluate('cuiGallery.exports.gallery_toolbar_activations(4)')
            page.keyboard.press('Control+a')
            page.keyboard.type('draft')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_toolbar_activations(4)>n', arg=before)
            wait_text('Find: 1 matching line')
            assert 'Find: 1 matching line' in text()
            assert 'Match: Save your draft before sharing it.' in text()
            page.keyboard.press('Control+a')
            page.keyboard.type('absent')
            wait_text('No results in this document.')
            assert 'No results in this document.' in text()
            page.keyboard.press('Control+a')
            page.keyboard.press('Backspace')
            wait_text('Find: enter a search term')
            assert 'Find: enter a search term' in text()
            page.locator('#disabled').check()
            wait_disabled('Find', True)
            before = page.evaluate('cuiGallery.exports.gallery_toolbar_activations(4)')
            click('Find')
            page.keyboard.type('find')
            assert page.evaluate('cuiGallery.exports.gallery_toolbar_activations(4)') == before
            assert 'Find: enter a search term' in text()
            browser.close()
            print('PASS: intrinsic HoverCard, simulated window actions/fullscreen, real Find edits/results/disabled/shortcut')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
