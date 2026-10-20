#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Headless actual-WASM Avatar variants and compilation of the displayed C usage."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import tempfile
import threading
from playwright.sync_api import sync_playwright
from test_snippets import EXTRACT
from test_touch_components import SNAPSHOT

ROOT = Path(__file__).resolve().parents[2]
COMMANDS = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({f:Array.from({length:25},(_,f)=>e.gallery_command(i,f)),
 text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))}));}'''


def compile_usage():
    data = json.loads(subprocess.check_output(
        ['node', '-e', EXTRACT, str(ROOT / 'web/gallery.js')], text=True))
    baseline = {c['name'] for c in json.loads((ROOT / 'docs/gpui-parity.json').read_text())['components']}
    assert len(baseline) == 78 and set(data['stories']) == baseline | {'Video', 'VideoPlayer'}
    source = ('#include "cui/cui_components.h"\n#include "cui/cui_content.h"\n'
              'void usage(cui_context *ui, cui_id id, cui_theme theme) {\n'
              'cui_bitmap photo = {0};\n' + data['snippets']['Avatar'] + '\n}\n')
    with tempfile.TemporaryDirectory(prefix='cui-avatar-usage-') as directory:
        path = Path(directory) / 'usage.c'
        path.write_text(source)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                        '-fsyntax-only', '-I', str(ROOT / 'include'), str(path)], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', type=Path, default=ROOT / 'build/web')
    args = parser.parse_args()
    compile_usage()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for width, dpr in [(1280, 1), (390, 2)]:
                page = browser.new_page(viewport={'width': width, 'height': 1000}, device_scale_factor=dpr)
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Avatar')
                page.wait_for_function('window.cuiGallery&&cuiGallery.renderedComponent==="Avatar"')

                def change(identifier, value):
                    frame = page.evaluate('cuiGallery.frame')
                    page.locator('#' + identifier).select_option(str(value))
                    page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                    page.evaluate('()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')

                def main_avatar(name='Taylor Reed'):
                    matches = [s for s in page.evaluate(SNAPSHOT) if s['name'] == name]
                    assert len(matches) == 1, matches  # Placeholder creates no second semantic.
                    return matches[0]['f'][1:5]

                def inside(command, bounds):
                    x, y, w, h = command['f'][1:5]
                    bx, by, bw, bh = bounds
                    return x >= bx - .01 and y >= by - .01 and x + w <= bx + bw + .01 and y + h <= by + bh + .01

                def fill_color():
                    bounds = main_avatar()
                    fills = [c for c in page.evaluate(COMMANDS)
                             if c['f'][0] == 0 and all(abs(a-b) < .01 for a, b in zip(c['f'][1:5], bounds))]
                    assert len(fills) == 1, fills
                    color = fills[0]['f'][5:9]
                    pixel = page.evaluate('''(b)=>{const c=document.querySelector('canvas'),s=c.width/c.clientWidth;
                      return Array.from(c.getContext('2d').getImageData(Math.floor((b[0]+b[2]*.5)*s),
                       Math.floor((b[1]+b[3]*.15)*s),1,1).data);}''', bounds)
                    assert pixel == color, (pixel, color, bounds)
                    return color

                assert page.locator('#design-size').is_visible()
                assert len([c for c in page.evaluate(COMMANDS) if c['f'][0] == 5]) >= 4
                expected = [16, 24, 48, 48, 80, 96, 112, 128, 160, 208, 256, 304, 400, 512]
                for size, diameter in enumerate(expected, 1):
                    change('design-size', size)
                    bounds = main_avatar()
                    assert abs(bounds[2] - diameter) < .01 and bounds[3] == bounds[2], (size, bounds)
                    texts = [c for c in page.evaluate(COMMANDS) if c['text'] == 'TR']
                    assert len(texts) == 1 and inside(texts[0], bounds), (size, texts, bounds)
                    extent = page.locator('canvas').evaluate('(c)=>[c.clientWidth,c.clientHeight]')
                    assert bounds[0] + diameter <= extent[0] and bounds[1] + diameter <= extent[1]
                for size, rem in [(5, 512), (14, 64), (1, 512)]:
                    change('design-size', size)
                    change('avatar-rem', rem)
                    bounds = main_avatar()
                    assert abs(bounds[2] - 512) < .01, (size, rem, bounds)
                    text = next(c for c in page.evaluate(COMMANDS) if c['text'] == 'TR')
                    assert inside(text, bounds) and text['f'][11] < 512, (size, rem, text, bounds)

                change('avatar-rem', 16)
                change('design-size', 4)
                change('avatar-hue', 0)
                light = fill_color()
                change('avatar-hue', 6)
                assert fill_color() != light
                # Use the existing legacy theme configuration hook; actual WASM palette uses theme.dark.
                frame = page.evaluate('cuiGallery.frame')
                page.evaluate("()=>{const c=document.querySelector('#theme');c.value='1';c.dispatchEvent(new Event('change'));}")
                page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                change('avatar-hue', 0)
                assert fill_color() != light
                frame = page.evaluate('cuiGallery.frame')
                page.locator('#avatar-palette').uncheck()
                page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                neutral = fill_color()
                change('avatar-hue', 6)
                assert fill_color() == neutral
                frame = page.evaluate('cuiGallery.frame')
                page.locator('#avatar-palette').check()
                page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                assert fill_color() != neutral

                for placeholder in [0, 1, 2]:
                    change('avatar-source', 2)
                    change('avatar-placeholder', placeholder)
                    bounds = main_avatar('Anonymous')
                    images = [c for c in page.evaluate(COMMANDS) if c['f'][0] == 5 and inside(c, bounds)]
                    assert len(images) == 1 and abs(images[0]['f'][3] - bounds[2] * .6) < 1.01, (placeholder, images, bounds)
                    assert not [c for c in page.evaluate(COMMANDS) if c['text'] == 'TR']
                    if placeholder == 2:
                        pixel = page.evaluate('''b=>{const c=document.querySelector('canvas'),s=c.width/c.clientWidth;
                         return Array.from(c.getContext('2d').getImageData(Math.floor((b[0]+b[2]/2)*s),
                          Math.floor((b[1]+b[3]/2)*s),1,1).data);}''', bounds)
                        background = next(c['f'][5:9] for c in page.evaluate(COMMANDS)
                                          if c['f'][0] == 0 and c['f'][1:5] == bounds)
                        assert pixel[3] == 255 and pixel != background, (pixel, background)
                # Exercise more raster sizes than the retained-version limit; the
                # gallery collects only after the preceding draw has been painted.
                for size in range(1, 15):
                    change('design-size', size)
                    for rem in [16, 64]:
                        change('avatar-rem', rem)
                        bounds = main_avatar('Anonymous')
                        images = [c for c in page.evaluate(COMMANDS) if c['f'][0] == 5 and inside(c, bounds)]
                        assert len(images) == 1, (size, rem, images)
                change('avatar-rem', 16)
                change('design-size', 4)
                change('avatar-source', 1)
                bounds = main_avatar()
                assert not [c for c in page.evaluate(COMMANDS) if c['f'][0] == 5 and inside(c, bounds)]
                change('avatar-source', 0)
                bounds = main_avatar()
                images = [c for c in page.evaluate(COMMANDS) if c['f'][0] == 5 and inside(c, bounds)]
                assert len(images) == 1 and images[0]['f'][3:5] == bounds[2:4]
                assert not [c for c in page.evaluate(COMMANDS) if c['text'] == 'TR']
                assert not errors and page.evaluate('cuiGallery.exports.gallery_error()') == 0, errors
                print(f'PASS actual Avatar sizes/palette/SVG/photo/cap: width={width}, DPR={dpr}', flush=True)
                page.close()
            browser.close()
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
