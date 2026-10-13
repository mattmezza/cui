#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Chromium dimensions for the public design-size gallery controls."""
import argparse
import json
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


FAMILIES = ['DropdownButton','Badge','Tag','Attachment','Progress','Settings','Tabs',
            'DescriptionList','Icon','Toolbar','Questionnaire','Speech']
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1600, 'height': 1200})
            def choose(selector, value):
                # Dispatch and snapshot in one browser turn: no animation frame can
                # publish old dimensions between reading the counter and this change.
                frame = page.evaluate("""({selector,value})=>{const node=document.querySelector(selector);
                  const before=cuiGallery.frame;node.value=String(value);
                  node.dispatchEvent(new Event('change',{bubbles:true}));return before;}""",
                  {'selector':selector,'value':value})
                page.wait_for_function('(before)=>cuiGallery.frame>before', arg=frame)

            observations = {}
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            for family in FAMILIES:
                page.goto(f'http://127.0.0.1:{server.server_port}/#{family}')
                page.wait_for_function('(s)=>window.cuiGallery?.renderedComponent===s', arg=family)
                assert page.locator('#design-size').is_visible(), family
                assert page.locator('#design-size option').count() == 14
                legacy = {'DropdownButton':'dropdown-size','Badge':'control-size','Tag':'control-size',
                          'Attachment':'control-size','Progress':'control-size','Settings':'settings-size',
                          'Tabs':'tabs-size','DescriptionList':'description-size','Icon':'icon-size',
                          'Toolbar':'toolbar-size','Questionnaire':'questionnaire-size','Speech':'speech-size'}
                assert page.locator('#'+legacy[family]).is_hidden(), family
                rows = []
                for size in range(1,15):
                    choose('#design-size', str(size))
                    page.wait_for_timeout(100)
                    data = page.evaluate("""()=>{const e=cuiGallery.exports,d=new TextDecoder();
                      return {text:Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),
                        text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),
                        font:e.gallery_command(i,11),x:e.gallery_command(i,1),y:e.gallery_command(i,2),w:e.gallery_command(i,3),h:e.gallery_command(i,4)})),
                       semantics:Array.from({length:e.gallery_semantic_count()},(_,i)=>({role:e.gallery_semantic(i,0),
                         name:d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i))),
                         x:e.gallery_semantic(i,1),y:e.gallery_semantic(i,2),w:e.gallery_semantic(i,3),h:e.gallery_semantic(i,4)}))};}""")
                    rows.append({'size':size,**data})

                    font = [0,12,14,16,16,18,20,24,30,36,48,60,72,96,128][size]
                    height = [0,20,24,32,32,40,48,56,64,80,104,128,152,200,264][size]
                    def semantic(name, role=None):
                        return next(x for x in data['semantics'] if x['name'] == name
                                    and (role is None or x['role'] == role))
                    def text_font(prefix, expected):
                        assert any(x['text'].startswith(prefix) and abs(x['font'] - expected) < .02
                                   for x in data['text']), (family, size, prefix, expected, data['text'])
                    if family == 'DropdownButton':
                        assert semantic('Export workspace')['h'] == height
                        text_font('Export workspace', font)
                    elif family == 'Badge':
                        text_font('7', font * .75)
                        assert semantic('7')['h'] > 0
                    elif family == 'Tag':
                        text_font('Workspace', font)
                    elif family == 'Attachment':
                        text_font('Attachment 1', font)
                        assert semantic('Attachment 1')['h'] >= height
                    elif family == 'Progress':
                        assert semantic('Progress', 21)['h'] == height * .25, (family, size, semantic('Progress', 21), height * .25)
                    elif family == 'Settings':
                        text_font('Preferences', font)
                        assert semantic('Search settings')['h'] == height
                    elif family == 'Tabs':
                        assert semantic('Overview')['h'] == height
                        text_font('Overview', font)
                    elif family == 'DescriptionList':
                        text_font('Owner', font * .875)
                    elif family == 'Icon':
                        assert any(x['type'] == 5 and abs(x['w'] - font) < 1 for x in data['text'])
                    elif family == 'Toolbar':
                        assert semantic('Save')['h'] == height
                        text_font('Save', font)
                    elif family == 'Questionnaire':
                        text_font('Question', font * .75)
                        assert semantic('Build a native app')['h'] >= height
                    elif family == 'Speech':
                        assert semantic('Start dictation')['h'] == height
                observations[family] = [{'size':r['size']} for r in rows]



            # Circular variants have bounded raster storage for every design size.
            page.goto(f'http://127.0.0.1:{server.server_port}/#Progress')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Progress"')
            for shape in ['1','2']:
                choose('#progress-shape', shape)
                for size in range(1,15):
                    choose('#design-size', str(size))
                    page.wait_for_timeout(100)
                    height = [0,20,24,32,32,40,48,56,64,80,104,128,152,200,264][size]
                    diameter = height * (2 if shape == '2' else 1)
                    bounds = page.evaluate("""()=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_semantic_count();i++)
                        if(new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==="Progress"&&e.gallery_semantic(i,0)===21)return {w:e.gallery_semantic(i,3),h:e.gallery_semantic(i,4)};}""")
                    assert bounds and bounds['w'] == diameter and bounds['h'] == diameter, (shape,size,bounds,diameter)
                    painted = page.evaluate('''diameter=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i)
                      .some(i=>e.gallery_command(i,0)===5&&e.gallery_command(i,3)===diameter&&e.gallery_command(i,4)===diameter
                        &&e.gallery_bitmap(i,0)===diameter*2&&e.gallery_bitmap(i,1)===diameter*2);}''', diameter)
                    assert painted, (shape,size,diameter)
            # The same full scale is density-adjusted once.
            for family, name in [('DropdownButton','Export workspace'),('Toolbar','Save'),
                                  ('Tabs','Overview'),('Speech','Start dictation')]:
                page.goto(f'http://127.0.0.1:{server.server_port}/#{family}')
                page.wait_for_function('(s)=>window.cuiGallery?.renderedComponent===s', arg=family)
                choose('#design-size', '7')
                choose('#density', '1')
                page.wait_for_timeout(120)
                actual = page.evaluate("""name=>{const e=cuiGallery.exports,d=new TextDecoder();
                  for(let i=0;i<e.gallery_semantic_count();i++)
                    if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                      return e.gallery_semantic(i,4);}""", name)
                assert actual == 56 * .875, (family, actual)
            # Both overflow layouts retain a square, centered SVG trigger and operable menu.
            page.set_viewport_size({'width': 900, 'height': 1200})
            page.goto(f'http://127.0.0.1:{server.server_port}/#Tabs')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Tabs"')
            for mode in ['1','2']:
                choose('#tabs-mode', mode)
                choose('#design-size', '1')
                page.wait_for_timeout(120)
                button = page.evaluate("""()=>{const e=cuiGallery.exports,d=new TextDecoder();
                  for(let i=0;i<e.gallery_semantic_count();i++)if(d.decode(new Uint8Array(e.memory.buffer,
                    e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==='All tabs')
                    return {role:e.gallery_semantic(i,0),x:e.gallery_semantic(i,1),y:e.gallery_semantic(i,2),
                      w:e.gallery_semantic(i,3),h:e.gallery_semantic(i,4)};}""")
                assert button and button['role'] == 1 and button['w'] == button['h'] and button['w'] >= 32, button
                icons = page.evaluate("""()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>({
                    type:e.gallery_command(i,0),x:e.gallery_command(i,1),y:e.gallery_command(i,2),
                    w:e.gallery_command(i,3),h:e.gallery_command(i,4)})).filter(x=>x.type===5);}""")
                assert any(abs(x['x']+x['w']/2-button['x']-button['w']/2)<1 and
                           abs(x['y']+x['h']/2-button['y']-button['h']/2)<1 for x in icons), (button,icons)
                rect = page.locator('canvas').bounding_box()
                page.mouse.click(rect['x']+button['x']+button['w']/2,rect['y']+button['y']+button['h']/2)
                page.wait_for_timeout(100)
                assert page.locator('#accessible').get_by_role('menuitem', name='Overview', exact=True).count() == 1
                page.keyboard.press('Escape')
                page.wait_for_timeout(100)
            assert not errors, errors
            print(json.dumps({"families":list(observations),"sizes":14,"tabs_overflow_axes":2,"status":"PASS"}))
            browser.close()
    finally: server.shutdown()
if __name__ == '__main__': main()
