#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM opt-in Button phases and named control paint, pixels and input."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

SHADOWS = '''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i)
 .filter(i=>[7,8].includes(e.gallery_command(i,0))).map(i=>({type:e.gallery_command(i,0),
 v:Array.from({length:16},(_,f)=>e.gallery_shadow_value(i,f))}));}'''
BASELINE = '''()=>{const c=document.querySelector('canvas'),x=c.getContext('2d');
 window.__controlShadowBaseline=x.getImageData(0,0,c.width,c.height);}'''
PIXEL_DIFF = '''shadows=>{const c=document.querySelector('canvas'),ctx=c.getContext('2d'),
 base=window.__controlShadowBaseline,scale=ctx.getTransform().a;
 if(base.width!==c.width||base.height!==c.height)throw Error('baseline geometry changed');
 const now=ctx.getImageData(0,0,c.width,c.height);let changed=0;
 for(const {v} of shadows){
  const left=Math.max(0,Math.floor((v[0]-10)*scale)),top=Math.max(0,Math.floor((v[1]-10)*scale)),
   right=Math.min(c.width,Math.ceil((v[0]+v[2]+10)*scale)),
   bottom=Math.min(c.height,Math.ceil((v[1]+v[3]+10)*scale));
  for(let y=top;y<bottom;y++)for(let x=left;x<right;x++){
   const i=(y*c.width+x)*4;
   if([0,1,2].some(k=>Math.abs(now.data[i+k]-base.data[i+k])>=3))changed++;
  }
 }
 if(changed<8)throw Error('named shadow did not produce distinguishable painted pixels: '+changed);
 return changed;
}'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--report', type=Path)
    p.add_argument('--compile-usage', action='store_true', help='Requires an allocated compiler slot')
    args = p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    results = []
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for width, dpr in ((1280, 1), (390, 2)):
                page = browser.new_page(viewport={'width': width, 'height': 1000}, device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page, a.errors = page, []
                page.on('pageerror', lambda e: a.errors.append(str(e)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
                page.wait_for_function('window.cuiGallery&&cuiGallery.renderedComponent==="Button"')
                page.emulate_media(reduced_motion='reduce')
                def shadows(): return page.evaluate(SHADOWS)
                def clean():
                    assert page.evaluate('cuiGallery.exports.gallery_error()') == 0
                    assert page.evaluate('cuiGallery.exports.gallery_control_stats(14)') == 0
                    assert not a.errors, a.errors
                def painted():
                    values = shadows()
                    assert values and all(v['v'][2] > 0 and v['v'][3] > 0 for v in values)
                    pixels = page.evaluate(PIXEL_DIFF, values)
                    clean()
                    return values, pixels
                def code(name):
                    text = page.locator('#snippet').inner_text()
                    assert 'void shadow_example' in text and 'cui_' in text, (name, text)
                    if args.compile_usage:
                        from test_questionnaire_progress_gallery import compile_usage
                        compile_usage(text)
                for name, label in (('Button', 'Run action'), ('Checkbox', 'Receive updates'),
                                    ('Radio', 'Priority'), ('Switch', 'Notifications'), ('Slider', 'Volume')):
                    a.route(name)
                    if name == 'Slider':
                        page.evaluate('cuiGallery.exports.gallery_slider_legacy_fixture(1)')
                        a.configure('control-shadows', False)
                    page.mouse.move(1, 1)
                    page.wait_for_timeout(80)
                    assert not page.locator('#control-shadows').is_checked()
                    assert shadows() == [], (name, shadows())
                    if name == 'Radio':
                        label = 'Standard' if a.target('Priority')['f'][6] else 'Priority'
                    before = a.target(label)
                    geometry = before['f'][1:5]
                    page.evaluate(BASELINE)
                    a.configure('control-shadows', True)
                    assert a.target(label)['f'][1:5] == geometry, name
                    values, pixels = painted()
                    code(name)
                    results.append({'component': name, 'width': width, 'dpr': dpr,
                                    'painted_difference_pixels': pixels, 'layers': values})
                    point = a.point(a.target(label))
                    if name == 'Button':
                        # Pointer hover and press must actually resolve different policies.
                        page.mouse.move(point['x'], point['y'])
                        page.wait_for_timeout(80)
                        assert [(v['type'], v['v'][9], v['v'][10]) for v in shadows()] == [(7, 6, 3)]
                        clicks = page.evaluate('cuiGallery.exports.gallery_clicks()')
                        page.mouse.down()
                        page.wait_for_timeout(80)
                        assert [(v['type'], v['v'][9], v['v'][10]) for v in shadows()] == [(8, 2, 1)]
                        # Same pressed state, legacy paint baseline, then INSET pixels.
                        a.configure('control-shadows', False)
                        assert shadows() == []
                        page.evaluate(BASELINE)
                        a.configure('control-shadows', True)
                        painted()
                        page.mouse.up()
                        page.wait_for_timeout(80)
                        assert page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks + 1
                        a.configure('disabled', True)
                        assert shadows() == []  # Instance replay must retain explicit clear.
                        a.configure('control-shadows', False)
                        page.evaluate(BASELINE)
                        a.configure('control-shadows', True)
                        assert page.evaluate('''()=>{const c=document.querySelector('canvas'),
                          a=c.getContext('2d').getImageData(0,0,c.width,c.height),b=window.__controlShadowBaseline;
                          return a.data.every((v,i)=>v===b.data[i]);}''')
                        page.mouse.click(point['x'], point['y'])
                        assert page.evaluate('cuiGallery.exports.gallery_clicks()') == clicks + 1
                        a.configure('disabled', False)
                    elif name == 'Slider':
                        old = page.evaluate('cuiGallery.exports.gallery_control_stats(15)')
                        page.mouse.click(point['x'], point['y'])
                        expected = 0 if old == 100 else 100
                        page.keyboard.press('Home' if expected == 0 else 'End')
                        page.wait_for_timeout(80)
                        assert page.evaluate('cuiGallery.exports.gallery_control_stats(15)') == expected
                        assert old != expected and shadows()[0]['type'] == 8
                    else:
                        old = a.target(label)['f'][6]
                        page.mouse.click(point['x'], point['y'])
                        page.wait_for_timeout(80)
                        assert a.target(label)['f'][6] != old, name
                        clean()
                        assert shadows()
                    # Named indicators must remain aligned over design-token changes.
                    if name != 'Slider':
                        for size in (1, 6):
                            a.configure('design-size', size)
                            assert shadows() or name == 'Button' and page.locator('#disabled').is_checked()
                            clean()
                    a.configure('control-shadows', False)
                    assert shadows() == []
                    clean()
                page.close()
            browser.close()
    finally:
        server.shutdown()
    if args.report:
        args.report.write_text(json.dumps({'status': 'passed', 'cases': results}, indent=2) + '\n')
    print('PASS actual gallery control shadow paint, states, geometry, values and legacy-off routes')


if __name__ == '__main__': main()
