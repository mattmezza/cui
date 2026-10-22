#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual Popover anchors, copied mesh pixels, controls and route lifetime."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_questionnaire_progress_gallery import compile_usage

SNAPSHOT = '''()=>{const e=cuiGallery.exports,canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');
const commands=Array.from({length:e.gallery_count()},(_,i)=>{const f=n=>e.gallery_command(i,n),type=f(0),out={i,type,b:[1,2,3,4].map(f),color:[5,6,7,8].map(f),radius:f(12)?f(13):f(9)};
if(type===6){out.linear=[75,76,77,78].map(f);out.vertices=Array.from({length:f(71)},(_,j)=>[f(72)+f(74)*f(79+j*4),f(73)+f(74)*f(80+j*4)]);}
if(type===7)out.caster=Array.from({length:4},(_,j)=>e.gallery_shadow_value(i,j));return out;});
return {commands,width:canvas.clientWidth,height:canvas.height/ctx.getTransform().a,dpr:ctx.getTransform().a,error:e.gallery_error(),open:e.gallery_surface_open(3)};}'''


def expected(trigger, popup, viewport, anchor, rem=16, offset=None, arrow=True):
    x, y, w, h = trigger
    pw, ph = popup
    gap = (rem/4 if offset is None else offset)+(rem*3/8 if arrow else 0)
    if anchor < 6:
        x = [x, x+(w-pw)/2, x+w-pw][anchor % 3]
        y = y+h+gap if anchor < 3 else y-ph-gap
    else:
        x = x+w+gap if anchor == 6 else x-pw-gap
        y += (h-ph)/2
    return [max(12, min(viewport[0]-12-pw, x)), max(12, min(viewport[1]-12-ph, y)), pw, ph]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for mobile, dpr in ((False, 1), (False, 2), (True, 2)):
                context = browser.new_context(viewport={'width':390 if mobile else 1280,'height':1000},has_touch=mobile,device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page = context.new_page(); a.errors = []; a.active_touch = False
                a.page.set_default_timeout(5000)
                a.page.on('pageerror', lambda e:a.errors.append(str(e)))
                a.page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
                a.page.wait_for_function('window.cuiGallery?.renderedComponent === "Button"')
                a.page.emulate_media(reduced_motion='reduce')
                a.session = context.new_cdp_session(a.page)
                a.route('Popover')
                a.configure('theme','1')
                assert a.page.locator('#popover-mode').input_value() == '0'
                assert a.page.locator('#popover-anchor').is_disabled()
                compile_usage(a.page.locator('#snippet').inner_text())
                def activate(target):
                    if mobile:
                        a.tap(target)
                    else:
                        point = a.point(target); a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(80)
                def pixel_at(point):
                    return a.page.evaluate('''p=>{const c=document.querySelector('canvas').getContext('2d'),s=c.getTransform().a;return Array.from(c.getImageData(Math.floor(p[0]*s),Math.floor(p[1]*s),1,1).data);}''',point)
                def arrow_pixels(data):
                    vertices = [v for c in data['commands'] if c['type']==6 for v in c['vertices']]
                    bounds = [min(v[0] for v in vertices),min(v[1] for v in vertices),max(v[0] for v in vertices),max(v[1] for v in vertices)]
                    return a.page.evaluate('''b=>{const c=document.querySelector('canvas').getContext('2d'),s=c.getTransform().a,x=Math.floor(b[0]*s),y=Math.floor(b[1]*s);return Array.from(c.getImageData(x,y,Math.ceil(b[2]*s)-x,Math.ceil(b[3]*s)-y).data);}''',bounds)
                activate(a.target('Open account card'))
                base = a.page.evaluate(SNAPSHOT)
                assert base['open'] and not [c for c in base['commands'] if c['type']==6]
                a.configure('popover-mode', '1')
                def check(anchor, rem=16, offset=None, arrow=True):
                    a.page.wait_for_timeout(80)
                    data = a.page.evaluate(SNAPSHOT)
                    trigger = a.target('Open account card')['f'][1:5]
                    shadows = [c for c in data['commands'] if c['type']==7]
                    panel = shadows[0]['caster']; pw = 340 if data['width']>500 else data['width']-48
                    wanted = expected(trigger, [pw,190], [data['width'],data['height']], anchor, rem, offset, arrow)
                    assert all(abs(x-y)<=1 for x,y in zip(panel,wanted)), (anchor,trigger,panel,wanted)
                    meshes = [c for c in data['commands'] if c['type']==6]
                    assert bool(meshes)==arrow, (anchor,meshes)
                    assert data['error']==0 and not a.errors, a.errors
                    if arrow:
                        fill = meshes[0]; radius = next(c['radius'] for c in data['commands'] if c['type']==0 and c['b']==panel)
                        depth = rem*3/8
                        if anchor < 6:
                            target = trigger[0]+[0,trigger[2]/2,trigger[2]][anchor%3]
                            center = max(panel[0]+depth+radius,min(panel[0]+panel[2]-depth-radius,target))
                            tip = [center,panel[1]-depth if anchor<3 else panel[1]+panel[3]+depth]
                        else:
                            center = max(panel[1]+depth+radius,min(panel[1]+panel[3]-depth-radius,trigger[1]+trigger[3]/2))
                            tip = [panel[0]-depth if anchor==6 else panel[0]+panel[2]+depth,center]
                        assert any(all(abs(x-y)<.02 for x,y in zip(v,tip)) for v in fill['vertices']), (anchor,tip,fill)
                        # The arrow is emitted after the panel/background and content.
                        assert fill['i'] > max(c['i'] for c in data['commands'] if c['type']==2)
                        # Sample the center of the arrow and both sides of its
                        # join. These are actual Canvas pixels, away from strokes.
                        if anchor < 6:
                            base = panel[1] if anchor<3 else panel[1]+panel[3]
                            sign = -1 if anchor<3 else 1
                            points = [[center,base+sign*depth/3], [center,base+sign*.5], [center,base-sign*2]]
                        else:
                            base = panel[0] if anchor==6 else panel[0]+panel[2]
                            sign = -1 if anchor==6 else 1
                            points = [[base+sign*depth/3,center], [base+sign*.5,center], [base-sign*2,center]]
                        pixels = a.page.evaluate('''points=>{const c=document.querySelector('canvas').getContext('2d'),s=c.getTransform().a;return points.map(([x,y])=>Array.from(c.getImageData(Math.floor(x*s),Math.floor(y*s),1,1).data));}''', points)
                        color = next(c['color'] for c in data['commands'] if c['type']==0 and c['b']==panel)
                        assert pixels[0]==color and pixels[2]==color, (anchor,pixels,color,points)
                        if not a.page.locator('#popover-edge').is_checked():
                            assert pixels[1]==color, (anchor,pixels,color,points)
                    compile_usage(a.page.locator('#snippet').inner_text())
                    return data
                for anchor in range(8):
                    a.configure('popover-anchor', str(anchor)); check(anchor)
                a.configure('popover-anchor','1')
                for gap in (-24,0,24):
                    a.page.locator('#popover-custom-offset').check()
                    a.configure('popover-offset',str(gap)); check(1,offset=gap)
                a.page.locator('#popover-custom-offset').uncheck()
                a.configure('popover-rem','32'); check(1,rem=32)
                a.page.locator('#popover-arrow').uncheck(); check(1,rem=32,arrow=False)
                a.page.locator('#popover-arrow').check()
                a.configure('popover-rem','16')
                before = check(1)
                a.page.locator('#popover-edge').check(); a.page.wait_for_timeout(80)
                edge = check(1)
                assert before['commands'] != edge['commands']
                panel = next(c['caster'] for c in edge['commands'] if c['type']==7)
                assert abs(panel[1]+panel[3]-(edge['height']-12))<=1
                settled = a.page.evaluate('cuiGallery.frame')
                a.page.wait_for_timeout(120)
                assert a.page.evaluate('cuiGallery.frame')==settled, 'anchor correction did not become idle'
                a.page.locator('#popover-edge').uncheck(); a.page.wait_for_timeout(80)
                ordinary = check(1)
                triangle = next(c['vertices'][:3] for c in ordinary['commands'] if c['type']==6)
                arrow_point = [sum(v[i] for v in triangle)/3 for i in (0,1)]
                opaque_pixel = pixel_at(arrow_point)
                opaque_region = arrow_pixels(ordinary)
                profile = a.target('View profile')
                a.page.locator('#popover-inert').check(); a.page.wait_for_timeout(80)
                inert = a.page.evaluate(SNAPSHOT)
                assert [c for c in inert['commands'] if c['type']==6] == [c for c in ordinary['commands'] if c['type']==6]
                assert pixel_at(arrow_point)==opaque_pixel, 'inert decoration pixel changed'
                assert arrow_pixels(inert)==opaque_region
                assert not any(n['name'] in ('Alex Morgan','View profile') for n in a.snapshot())
                activate(profile)
                assert a.page.evaluate(SNAPSHOT)['open'], 'inert profile activation'
                a.page.locator('#popover-inert').uncheck(); a.page.wait_for_timeout(80)
                a.configure('popover-opacity','0.5')
                faded = a.page.evaluate(SNAPSHOT)
                assert next(c['linear'][3] for c in faded['commands'] if c['type']==6)==.5
                assert all(abs(c['linear'][3]-old['linear'][3]*.5)<1e-6 for c,old in zip(
                    [c for c in faded['commands'] if c['type']==6],
                    [c for c in ordinary['commands'] if c['type']==6]))
                assert any(c['color'][3]==127 for c in faded['commands'] if c['type']==0)
                assert arrow_pixels(faded)!=opaque_region, 'opacity did not reach arrow pixels'
                compile_usage(a.page.locator('#snippet').inner_text())
                a.configure('popover-opacity','1')
                a.configure('popover-anchor','7')
                a.page.locator('#popover-edge').check(); a.page.wait_for_timeout(80)
                below = check(7)
                # Right-center at the lower edge stays outside the centered
                # modal panel, so its actual pixel must receive only backdrop.
                fill = next(c for c in below['commands'] if c['type']==6)
                max_x = max(v[0] for v in fill['vertices'])
                min_x = min(v[0] for v in fill['vertices'])
                center_y = (min(v[1] for v in fill['vertices'])+max(v[1] for v in fill['vertices']))/2
                point = [max_x-(max_x-min_x)*2/3,center_y]
                pixel = a.page.evaluate('''p=>{const c=document.querySelector('canvas').getContext('2d'),s=c.getTransform().a;return Array.from(c.getImageData(Math.floor(p[0]*s),Math.floor(p[1]*s),1,1).data);}''',point)
                a.page.locator('#popover-modal').check(); a.page.wait_for_timeout(100)
                above = a.page.evaluate(SNAPSHOT)
                assert above['open'] and any(c['type']==6 for c in above['commands'])
                assert a.target('Close layering modal')
                modal_pixel = a.page.evaluate('''([p,rgba])=>{const c=document.querySelector('canvas').getContext('2d'),s=c.getTransform().a,r=new OffscreenCanvas(1,1).getContext('2d');r.putImageData(new ImageData(new Uint8ClampedArray(rgba),1,1),0,0);r.fillStyle='rgba(0,0,0,'+(100/255)+')';r.fillRect(0,0,1,1);return {actual:Array.from(c.getImageData(Math.floor(p[0]*s),Math.floor(p[1]*s),1,1).data),expected:Array.from(r.getImageData(0,0,1,1).data)};}''',[point,pixel])
                assert modal_pixel['actual']==modal_pixel['expected'], (point,modal_pixel)
                mesh_last = max(c['i'] for c in above['commands'] if c['type']==6)
                assert any(c['i']>mesh_last and c['type']==0 and c['color']==[0,0,0,100] for c in above['commands'])
                compile_usage(a.page.locator('#snippet').inner_text())
                activate(a.target('Close layering modal'))
                assert not a.page.locator('#popover-modal').is_checked()
                a.configure('popover-mode','0')
                legacy = a.page.evaluate(SNAPSHOT)
                assert not [c for c in legacy['commands'] if c['type']==6]
                a.route('Button'); a.route('Popover')
                assert not a.page.evaluate(SNAPSHOT)['open']
                assert a.page.locator('#popover-mode').input_value() == '0'
                settled = a.page.evaluate('cuiGallery.frame')
                a.page.wait_for_timeout(120)
                assert a.page.evaluate('cuiGallery.frame')==settled
                assert not a.errors, a.errors
                print(f'PASS actual anchors/offsets/rem/arrow/clamp/legacy/teardown mobile={mobile} DPR={dpr}',flush=True)
                context.close()
            browser.close()
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
