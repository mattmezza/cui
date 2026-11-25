#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual-WASM HoverCard policies through browser mouse, keyboard and touch input."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread
from playwright.sync_api import sync_playwright
from test_questionnaire_progress_gallery import compile_usage


SEMANTIC_BOUNDS = '''name=>{const e=cuiGallery.exports,d=new TextDecoder();
    for(let i=0;i<e.gallery_semantic_count();i++){
        if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),
                                  e.gallery_semantic_length(i)))===name)
            return [1,2,3,4].map(f=>e.gallery_semantic(i,f));
    }
    return null;
}'''


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
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 1000}, has_touch=True)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#HoverCard')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="HoverCard"')
            compile_usage(page.locator('#snippet').inner_text())
            page.locator('canvas').scroll_into_view_if_needed()
            cdp = page.context.new_cdp_session(page)

            def wait_open(value):
                page.wait_for_function('(v)=>Boolean(cuiGallery.exports.gallery_hover_state(2))===v', arg=value)

            def bounds(name):
                return page.evaluate(SEMANTIC_BOUNDS, name)

            def point(name):
                b = bounds(name)
                assert b, name
                canvas = page.locator('canvas').bounding_box()
                return {'x': canvas['x']+b[0]+b[2]/2, 'y': canvas['y']+b[1]+b[3]/2}

            def outside():
                c = page.locator('canvas').bounding_box()
                return {'x': c['x']+c['width']-8, 'y': c['y']+c['height']-8}

            def click(name):
                page.mouse.click(**point(name))

            def policy(value):
                page.locator('#hover-policy').select_option(str(value))
                page.wait_for_function('(v)=>document.querySelector("#snippet").textContent.includes(v)',
                                       arg='CUI_HOVER_CARD_RELEASE_RAW_PRIMARY' if value == 2 else 'CUI_HOVER_CARD_RELEASE_LEGACY')
                wait_open(False)

            def touch(kind, points):
                cdp.send('Input.dispatchTouchEvent', {'type': kind, 'touchPoints': points})
                page.wait_for_timeout(60)

            # Strict tap keeps inside-down / far-outside-up; raw policy closes it.
            for mode in (1, 2):
                policy(mode)
                compile_usage(page.locator('#snippet').inner_text())
                click('Hover or focus here')
                wait_open(True)
                assert page.evaluate('cuiGallery.exports.gallery_hover_state(7)') == 1
                before = page.evaluate('cuiGallery.exports.gallery_hover_state(6)')
                page.mouse.move(**point('Profile details'))
                page.mouse.down()
                page.mouse.move(**outside(), steps=5)
                page.mouse.up()
                wait_open(mode == 1)
                assert page.evaluate('cuiGallery.exports.gallery_hover_state(6)') == before + (mode == 2)
                if mode == 1:
                    page.keyboard.press('Escape')
                    wait_open(False)

            # Accepted trigger toggles exactly once; inside release preserves content.
            click('Hover or focus here')
            wait_open(True)
            click('Profile details')
            wait_open(True)
            click('Hover or focus here')
            wait_open(False)
            page.keyboard.press('Enter')
            wait_open(True)
            page.keyboard.press('Escape')
            wait_open(False)

            # Real CDP touch activation and outside tap use the same raw C path.
            t = {'id': 1, **point('Hover or focus here')}
            touch('touchStart', [t])
            touch('touchEnd', [])
            wait_open(True)
            t = {'id': 2, **outside()}
            touch('touchStart', [t])
            touch('touchEnd', [])
            wait_open(False)

            def card_height(minimum, maximum):
                page.wait_for_function(f'''([lo, hi])=>{{
                    const card=({SEMANTIC_BOUNDS})('Profile details');
                    return card && card[3] > lo && card[3] < hi;
                }}''', arg=[minimum, maximum])
                return bounds('Profile details')[3]

            def wrapped_content_fits():
                x, y, w, h = bounds('Profile details')
                canvas = page.locator('canvas').bounding_box()
                assert x >= 0 and y >= 0 and x + w <= canvas['width'] + .1
                assert y + h <= canvas['height'] + .1
                lines = page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                    return Array.from({length:e.gallery_count()},(_,i)=>i)
                        .filter(i=>e.gallery_command(i,0)===2)
                        .map(i=>[d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
                                                       e.gallery_command_length(i))),
                                 ...[1,2,3,4].map(f=>e.gallery_command(i,f))]);}''')
                profile = [line for line in lines if line[0] != 'Hover or focus here']
                assert len(profile) >= 3 and profile[0][0] == 'Alex Morgan', profile
                assert profile[-1][0].endswith('use them.'), profile
                for _, tx, ty, tw, th in profile:
                    assert tx >= x and ty >= y and tx + tw <= x + w + .1
                    assert ty + th < y + h

            # Update the body while the same tap-open card is still visible.
            click('Hover or focus here')
            wait_open(True)
            compact = card_height(30, 100)
            compile_usage(page.locator('#snippet').inner_text())
            changes = page.evaluate('cuiGallery.exports.gallery_hover_state(6)')
            page.locator('#hover-preview').select_option('1')
            card_height(compact + 8, 180)
            wrapped_content_fits()
            compile_usage(page.locator('#snippet').inner_text())
            page.locator('#hover-preview').select_option('0')
            card_height(compact - .1, compact + .1)
            wait_open(True)
            assert page.evaluate('cuiGallery.exports.gallery_hover_state(6)') == changes

            # At a narrow width, touching a low trigger must reveal the whole
            # bio above it. Leaving that route must discard the open card.
            page.set_viewport_size({'width': 390, 'height': 844})
            page.locator('#hover-preview').select_option('2')
            wait_open(False)
            page.locator('canvas').scroll_into_view_if_needed()
            page.wait_for_function(f'''()=>({SEMANTIC_BOUNDS})('Hover or focus here')?.[1]>130''')
            t = {'id': 3, **point('Hover or focus here')}
            touch('touchStart', [t])
            touch('touchEnd', [])
            wait_open(True)
            page.wait_for_function(f'''()=>{{
                const bounds={SEMANTIC_BOUNDS};
                const card=bounds('Profile details'), trigger=bounds('Hover or focus here');
                return card && trigger && card[1]+card[3] <= trigger[1];
            }}''')
            wrapped_content_fits()
            compile_usage(page.locator('#snippet').inner_text())
            for story in ['Tooltip', 'HoverCard']:
                page.evaluate('(story)=>location.hash=story', story)
                page.wait_for_function('(story)=>cuiGallery.renderedComponent===story', arg=story)
            wait_open(False)
            assert bounds('Profile details') is None
            assert not errors, errors
            browser.close()
            print('PASS: actual WASM HoverCard policies, live content fit, narrow edge placement and route cleanup')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
