#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM Stepper requests and Canvas connector/icon evidence; no desktop."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import tempfile
import threading
from playwright.sync_api import sync_playwright

TARGET = '''name=>{const w=cuiGallery.exports,d=new TextDecoder();
 for(let i=0;i<w.gallery_semantic_count();i++){
 const nameAt=d.decode(new Uint8Array(w.memory.buffer,w.gallery_semantic_name(i),w.gallery_semantic_length(i)));
 if(nameAt===name)return {i,x:w.gallery_semantic(i,1),y:w.gallery_semantic(i,2),w:w.gallery_semantic(i,3),h:w.gallery_semantic(i,4)};
 }throw Error('Missing '+name);}'''
COMMANDS = '''()=>{const w=cuiGallery.exports,d=new TextDecoder();return Array.from({length:w.gallery_count()},(_,i)=>({
 i,type:w.gallery_command(i,0),x:w.gallery_command(i,1),y:w.gallery_command(i,2),w:w.gallery_command(i,3),h:w.gallery_command(i,4),
 color:[5,6,7,8].map(f=>w.gallery_command(i,f)),radius:w.gallery_command(i,9),
 text:d.decode(new Uint8Array(w.memory.buffer,w.gallery_command_text(i),w.gallery_command_length(i))),
 bitmap:Array.from({length:14},(_,f)=>w.gallery_bitmap(i,f))}));}'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    snippets = []
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            for width, dpr, touch in ((1440, 1, False), (1440, 2, False), (390, 2, True)):
                context = browser.new_context(viewport={'width': width, 'height': 1000 if not touch else 844},
                                              device_scale_factor=dpr, has_touch=touch, is_mobile=touch)
                page = context.new_page()
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Stepper')
                page.wait_for_function('window.cuiGallery?.renderedComponent==="Stepper"')
                def commands():
                    return page.evaluate(COMMANDS)
                def painted(text):
                    page.wait_for_function('(text)=>('+COMMANDS+')().some(c=>c.text.includes(text))', arg=text)
                def change(id, value):
                    before = page.evaluate('cuiGallery.frame')
                    page.locator('#'+id).select_option(str(value))
                    page.wait_for_function('(f)=>cuiGallery.frame>f', arg=before)
                def click(name):
                    t = page.evaluate(TARGET, name)
                    page.locator('#preview').scroll_into_view_if_needed()
                    box = page.locator('#preview').bounding_box()
                    x, y = box['x']+t['x']+t['w']/2, box['y']+t['y']+t['h']/2
                    before = page.evaluate('cuiGallery.frame')
                    if touch:
                        page.touchscreen.tap(x, y)
                    else:
                        page.mouse.click(x, y)
                    page.wait_for_function('(f)=>cuiGallery.frame>f', arg=before)
                def sample(command, fx=.5, fy=.5):
                    return page.evaluate('''([c,fx,fy])=>{const a=document.querySelector('#preview'),d=a.width/a.clientWidth;
                     return [...a.getContext('2d').getImageData(Math.floor((c.x+c.w*fx)*d),Math.floor((c.y+c.h*fy)*d),1,1).data];}''', [command, fx, fy])
                def geometry(vertical, diameter=24, thickness=2, span=20, icons=0):
                    # Requests update the owner for the next submission. Advance
                    # that ordinary paint frame before comparing pixels/commands.
                    page.evaluate('cuiGallery.render();cuiGallery.render()')
                    cs = commands()
                    separators = [c for c in cs if c['type'] == 0 and
                                  abs(c['w']-(thickness if vertical else span)) < .01 and
                                  abs(c['h']-(span if vertical else thickness)) < .01]
                    assert len(separators) == 2, (width, dpr, [c for c in cs if c['type']==0])
                    indicators = [c for c in cs if c['type'] == 0 and
                                  abs(c['w']-diameter) < .01 and abs(c['h']-diameter) < .01 and
                                  abs(c['radius']-diameter/2) < .01]
                    assert len(indicators) == 3, (width, dpr, indicators)
                    for c in separators:
                        px = sample(c)
                        # Fractional XS/scale strokes cover only part of a physical
                        # pixel. Compare the visible composite against area coverage.
                        bg_command = dict(c)
                        bg_command['x' if vertical else 'y'] -= 3
                        bg = sample(bg_command)
                        dx, dy = int((c['x']+c['w']/2)*dpr), int((c['y']+c['h']/2)*dpr)
                        coverage = max(0, min(dx+1, (c['x']+c['w'])*dpr)-max(dx, c['x']*dpr)) * max(0, min(dy+1, (c['y']+c['h'])*dpr)-max(dy, c['y']*dpr))
                        expected = [round(c['color'][i]*coverage+bg[i]*(1-coverage)) for i in range(4)]
                        assert all(abs(px[i]-expected[i]) <= 3 for i in range(4)), (px, expected, c)
                    images = [c for c in cs if c['type'] == 5 and c['bitmap'][:3] == [8, 8, 32]]
                    assert len(images) == icons, images
                    ordinals = [c for c in cs if c['type']==2 and c['text'] in ('1','2','3')]
                    assert len(ordinals) == (3 if not icons and page.locator('#stepper-density').input_value()!='1' else 0)
                    if icons:
                        assert images[0]['w'] == diameter and images[0]['h'] == diameter
                        assert images[0]['bitmap'][9] == 1
                        tint = images[0]['bitmap'][10:14]
                        solid, empty = sample(images[0], 4.5/8, 4.5/8), sample(images[0], 2.5/8, 2.5/8)
                        assert all(abs(solid[i]-tint[i]) <= 8 for i in range(4)), (solid, tint)
                        assert sum(abs(solid[i]-empty[i]) for i in range(3)) > 60, (solid, empty)
                    return separators, indicators
                # Default legacy story and validation behavior stay available.
                assert page.locator('#stepper-mode').input_value() == '0'
                assert not page.locator('#stepper-density').is_visible()
                snippets.append(page.locator('#snippet').inner_text())
                painted('Step 1 of 3')
                change('stepper-mode', 1)
                assert page.locator('#stepper-density').is_visible()
                geometry(True)
                snippets.append(page.locator('#snippet').inner_text())
                click('Review')
                painted('Navigation requests: 1')
                painted('Confirm details to continue.')
                painted('Step 1 of 3')
                click('Details')
                painted('Navigation requests: 2')
                click('Details confirmed')
                click('Review')
                painted('Step 2 of 3')
                painted('Navigation requests: 3')
                separators, _ = geometry(True)
                assert separators[0]['color'] != separators[1]['color']
                # Keyboard dispatch uses the real previous semantic target.
                target = page.evaluate(TARGET, 'Finish')
                page.evaluate('(i)=>{cuiGallery.exports.gallery_focus(i);cuiGallery.render();}', target['i'])
                page.locator('#preview').focus()
                page.keyboard.press('Enter')
                painted('Step 3 of 3')
                painted('Navigation requests: 4')
                page.keyboard.press('Space')
                painted('Navigation requests: 5')
                page.locator('#disabled').check()
                click('Details')
                painted('Navigation requests: 5')
                painted('Step 3 of 3')
                page.locator('#disabled').uncheck()
                change('stepper-icons', 1)
                geometry(True, icons=3)
                snippets.append(page.locator('#snippet').inner_text())
                for density, side, stroke in ((1, 8, 1.5), (2, 18, 2), (3, 32, 3), (0, 24, 2)):
                    change('stepper-density', density)
                    geometry(True, side, stroke, icons=0 if density == 1 else 3)
                # Shared LG metrics scale source proportions; all tokens remain selectable.
                assert page.locator('#stepper-design option').count() == 15
                change('stepper-design', 5)
                geometry(True, 27, 2.25, 22.5, 3)
                snippets.append(page.locator('#snippet').inner_text())
                change('stepper-design', 0)
                if not touch:
                    change('stepper-axis', 0)
                    geometry(False, span=36, icons=3)
                    snippets.append(page.locator('#snippet').inner_text())
                # Narrow source layout is not represented as completed responsive flex.
                assert page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                # Leave every route-owned choice nondefault before SPA navigation.
                change('stepper-axis', 0)
                change('stepper-density', 3)
                change('stepper-design', 5)
                page.evaluate("location.hash='Kbd'")
                page.wait_for_function('window.cuiGallery?.renderedComponent==="Kbd"')
                assert not page.locator('#stepper-controls').is_visible()
                page.evaluate("location.hash='Stepper'")
                page.wait_for_function('window.cuiGallery?.renderedComponent==="Stepper"')
                painted('Step 3 of 3')  # SPA route keeps owner data, resets only controls.
                assert page.locator('#stepper-mode').input_value() == '0'
                assert page.locator('#stepper-axis').input_value() == '1'
                assert page.locator('#stepper-density').input_value() == '0'
                assert page.locator('#stepper-icons').input_value() == '0'
                assert page.locator('#stepper-design').input_value() == '0'
                assert not errors, errors
                context.close()
            browser.close()
        # Compile exactly the displayed snippets, not a parallel hand-written example.
        with tempfile.TemporaryDirectory() as temporary:
            for i, code in enumerate(dict.fromkeys(snippets)):
                source = Path(temporary)/f'stepper-{i}.c'
                source.write_text(code)
                subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                '-I'+str(args.source_root.resolve()/'include'), '-c', str(source),
                                '-o', str(source.with_suffix('.o'))], check=True)
        print('PASS: desktop DPR1/2 + touch390 DPR2 WASM requests, validation, disabled, connector/icon Canvas pixels, source/shared sizes, route reset and compiled snippets')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
