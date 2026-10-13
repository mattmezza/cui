#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real CDP touch contacts against the WASM gallery (no mouse substitution)."""
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
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, has_touch=True)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
            page.wait_for_function('window.cuiGallery && cuiGallery.renderedComponent==="Button"')
            session = page.context.new_cdp_session(page)
            def semantic(name=None, scroll=False):
                return page.evaluate('''({name,scroll})=>{const e=cuiGallery.exports,d=new TextDecoder();
                    const i=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>scroll?e.gallery_semantic(i,17):d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name);
                    if(i===undefined)throw Error('Missing touch target '+name);
                    const r=document.querySelector('canvas').getBoundingClientRect();
                    return {i,x:r.left+e.gallery_semantic(i,1)+Math.min(50,e.gallery_semantic(i,3)/2),
                    y:r.top+e.gallery_semantic(i,2)+Math.min(70,e.gallery_semantic(i,4)/2)};}''', {'name': name, 'scroll': scroll})
            def dispatch(kind, points):
                session.send('Input.dispatchTouchEvent', {'type': kind, 'touchPoints': points})
                page.wait_for_timeout(40)
            def contact(target, identifier=1, dx=0, dy=0):
                return {'id': identifier, 'x': target['x']+dx, 'y': target['y']+dy}
            button = semantic('Run action')
            before = page.evaluate('cuiGallery.exports.gallery_clicks()')
            dispatch('touchStart', [contact(button)])
            dispatch('touchEnd', [])
            page.wait_for_function('(v)=>cuiGallery.exports.gallery_clicks()===v+1', arg=before)
            before += 1
            dispatch('touchStart', [contact(button)])
            dispatch('touchMove', [contact(button, dy=12)])
            dispatch('touchMove', [contact(button)])
            dispatch('touchEnd', [])
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == before, 'Drag produced a ghost tap'
            dispatch('touchStart', [contact(button)])
            dispatch('touchCancel', [])
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == before
            page.evaluate('location.hash="Scrollable"')
            page.wait_for_function('cuiGallery.renderedComponent==="Scrollable"')
            page.locator('canvas').scroll_into_view_if_needed()
            scroll = semantic(scroll=True)
            initial = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,19)', scroll['i'])
            dispatch('touchStart', [contact(scroll)])
            dispatch('touchMove', [contact(scroll, dy=-5)])
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,19)', scroll['i']) == initial
            dispatch('touchMove', [contact(scroll, dy=-35)])
            dispatch('touchEnd', [])
            page.wait_for_function('({i,initial})=>Math.abs(cuiGallery.exports.gallery_semantic(i,19)-initial-35)<.1', arg={'i': scroll['i'], 'initial': initial})
            page.evaluate('location.hash="Plot"')
            page.wait_for_function('cuiGallery.renderedComponent==="Plot"')
            page.locator('canvas').scroll_into_view_if_needed()
            plot = semantic('Projected sine wave')
            def domain():
                return page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))).find(t=>t.startsWith('Domain:'));}''')
            page.mouse.move(plot['x'], plot['y'])
            page.wait_for_function('document.querySelector("canvas").style.cursor==="grab"')
            old_domain = domain()
            dispatch('touchStart', [contact(plot, 1), contact(plot, 2, dx=50)])
            dispatch('touchMove', [contact(plot, 1), contact(plot, 2, dx=100)])
            dispatch('touchEnd', [])
            assert domain() != old_domain, 'Opt-in viewport pinch did not zoom'
            assert not errors, errors
            browser.close()
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
