#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Strict Plot viewport checks through actual mouse and Chromium CDP contacts."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
import math
from pathlib import Path
import threading
import tempfile
from playwright.sync_api import sync_playwright
from test_touch_components import Audit


def domains(a):
    return a.page.evaluate('()=>[0,1].map(axis=>[0,1].map(end=>cuiGallery.exports.gallery_plot_domain(axis,end)))')


def near(a, b):
    assert math.isclose(a, b, rel_tol=1e-5, abs_tol=1e-5), (a, b)


def raster(a, target):
    return a.page.evaluate('''(f)=>{const c=document.querySelector('canvas'),scale=c.width/c.clientWidth;
     const pixels=c.getContext('2d').getImageData(Math.round((f[1]+30)*scale),
      Math.round((f[2]+14)*scale),Math.round((f[3]-44)*scale),Math.round(124*scale)).data;
     let hash=2166136261;for(const byte of pixels)hash=Math.imul(hash^byte,16777619);return hash>>>0;}''', target['f'])


def setup(a):
    a.route('Plot')
    a.page.locator('#disabled').uncheck()
    a.tap(a.target('Reset', 1))
    a.page.evaluate('''()=>{window.plotInputLog=[];if(window.plotInputRecorder)return;
     window.plotInputRecorder=true;const canvas=document.querySelector('canvas');
     for(const type of ['mousedown','pointermove','wheel'])canvas.addEventListener(type,e=>{
      const r=canvas.getBoundingClientRect();plotInputLog.push({type,x:e.clientX-r.left,y:e.clientY-r.top});
     });}''')
    return a.target('Projected sine wave', 20)


def run(a):
    def shift_drag():
        target = setup(a)
        point = a.point(target, .45, .5)
        before = domains(a)
        a.page.keyboard.down('Shift')
        a.page.mouse.move(point['x'], point['y'])
        a.page.mouse.down()
        a.page.mouse.move(point['x']+60, point['y']+20, steps=4)
        a.page.wait_for_timeout(100)
        a.page.mouse.up()
        a.page.keyboard.up('Shift')
        after = domains(a)
        log=a.page.evaluate('plotInputLog')
        down=next(event for event in log if event['type']=='mousedown')
        move=[event for event in log if event['type']=='pointermove'][-1]
        dx,dy=move['x']-down['x'],move['y']-down['y']
        assert dx>59 and dy>19, (dx,dy)
        near(after[0][0]-before[0][0], -dx*100/(target['f'][3]-40))
        near(after[1][0]-before[1][0], dy*2.4/130)
        near(after[0][1]-after[0][0], 100)
        a.page.wait_for_function('document.querySelector("canvas").style.cursor==="grab"')
    a.case('Plot', 'Shift-primary drag pans both axes with exact projected distance', shift_drag)

    def ordinary_drag():
        target = setup(a)
        point = a.point(target)
        before = domains(a)
        a.page.mouse.move(point['x'], point['y'])
        a.page.mouse.down()
        a.page.mouse.move(point['x']+40, point['y']+10, steps=3)
        a.page.mouse.up()
        a.page.wait_for_timeout(100)
        assert domains(a) == before
    a.case('Plot', 'ordinary mouse drag preserves viewport', ordinary_drag)

    def ctrl_wheel():
        target = setup(a)
        point = a.point(target, .35, .4)
        before = domains(a)
        a.page.mouse.move(point['x'], point['y'])
        a.page.keyboard.down('Control')
        a.page.mouse.wheel(0, -120)
        a.page.keyboard.up('Control')
        a.page.wait_for_timeout(150)
        after = domains(a)
        wheel=a.page.evaluate('plotInputLog.filter(e=>e.type==="wheel").at(-1)')
        fx = (wheel['x']-target['f'][1]-28)/(target['f'][3]-40)
        fy = (wheel['y']-target['f'][2]-142)/-130
        for axis, fraction in enumerate([fx, fy]):
            near(after[axis][1]-after[axis][0], (before[axis][1]-before[axis][0])/math.exp(.24))
            near(after[axis][0]+fraction*(after[axis][1]-after[axis][0]),
                 before[axis][0]+fraction*(before[axis][1]-before[axis][0]))
        near(a.page.evaluate('visualViewport.scale'), 1)
    a.case('Plot', 'Ctrl-wheel zoom preserves actual screen/data anchor on both axes', ctrl_wheel)

    def wheel_passes():
        target = setup(a)
        point = a.point(target)
        before = domains(a)
        a.page.mouse.move(point['x'], point['y'])
        page_before=a.page.evaluate('scrollY')
        a.page.mouse.wheel(0, 120)
        a.page.wait_for_timeout(150)
        assert domains(a) == before
        assert a.page.evaluate('scrollY')>page_before+1, 'Plain Plot wheel blocked native page scrolling'
    a.case('Plot', 'plain wheel leaves viewport unchanged', wheel_passes)

    def touch_pan():
        target = setup(a)
        before = domains(a)
        a.drag(target, 40, 20)
        after = domains(a)
        near(after[0][0]-before[0][0], -40*100/(target['f'][3]-40))
        near(after[1][0]-before[1][0], 20*2.4/130)
        near(after[0][1]-after[0][0], 100)
    a.case('Plot', 'actual one-contact pan moves both data axes', touch_pan)

    def pinch():
        target = setup(a)
        center = a.point(target, .5, .5)
        before = domains(a)
        left = {**center, 'x': center['x']-40}
        right = {**center, 'id': 2, 'x': center['x']+40}
        a.dispatch('touchStart', [left, right])
        for step in range(1, 5):
            a.dispatch('touchMove', [{**left, 'x': left['x']-10*step},
                                     {**right, 'x': right['x']+10*step}])
        a.dispatch('touchEnd', [])
        after = domains(a)
        for axis in range(2):
            near(after[axis][1]-after[axis][0], (before[axis][1]-before[axis][0])/2)
        fx = (target['f'][3]*.5-28)/(target['f'][3]-40)
        near(after[0][0]+fx*(after[0][1]-after[0][0]),100*fx)
        a.tap(a.target('Reset',1))
        assert domains(a) == before
    a.case('Plot', 'actual two-contact pinch doubles zoom and Reset restores domains', pinch)

    def moving_pinch():
        target=setup(a)
        center=a.point(target,.5,.5)
        left={**center,'x':center['x']-40}
        right={**center,'id':2,'x':center['x']+40}
        a.dispatch('touchStart',[left,right])
        for step in range(1,5):
            a.dispatch('touchMove',[{**left,'y':left['y']+10*step/4},
                {**right,'x':right['x']+40*step/4,'y':right['y']+10*step/4}])
        a.dispatch('touchEnd',[])
        after=domains(a)
        old_fx=(target['f'][3]*.5-28)/(target['f'][3]-40)
        new_fx=(target['f'][3]*.5+20-28)/(target['f'][3]-40)
        old_fy=(90-142)/-130
        new_fy=(100-142)/-130
        near(after[0][1]-after[0][0],100/1.5)
        near(after[1][1]-after[1][0],2.4/1.5)
        near(after[0][0]+new_fx*(after[0][1]-after[0][0]),100*old_fx)
        near(after[1][0]+new_fy*(after[1][1]-after[1][0]),-1.2+2.4*old_fy)
    a.case('Plot','moving actual pinch preserves old-center data at the new center',moving_pinch)

    def capture_loss():
        target = setup(a)
        point = a.point(target)
        a.page.keyboard.down('Shift')
        a.page.mouse.move(point['x'], point['y'])
        a.page.mouse.down()
        a.page.mouse.move(point['x']+20,point['y'],steps=2)
        a.page.wait_for_timeout(100)
        before=domains(a)
        # Release the real capture created by the actual pointerdown.
        a.page.evaluate('()=>document.querySelector("canvas").releasePointerCapture(1)')
        a.page.mouse.move(point['x']+60,point['y'],steps=2)
        a.page.mouse.up()
        a.page.keyboard.up('Shift')
        a.page.wait_for_timeout(100)
        assert domains(a)==before
    a.case('Plot', 'unexpected mouse capture loss ends pan without ghost continuation', capture_loss)

    def touch_cancel():
        target=setup(a)
        point=a.point(target)
        a.dispatch('touchStart',[point])
        a.dispatch('touchMove',[{**point,'x':point['x']+20}])
        a.dispatch('touchCancel',[])
        before=domains(a)
        a.page.mouse.move(point['x']+60,point['y'])
        a.page.wait_for_timeout(100)
        assert domains(a)==before
        a.tap(a.target('Reset',1))
        assert domains(a)==[[0,100],[-1.2,1.2]]
    a.case('Plot', 'touch cancellation stops pan and next actual tap remains usable', touch_cancel)

    def disabled():
        target=setup(a)
        before=domains(a)
        a.page.locator('#disabled').check()
        a.page.wait_for_timeout(100)
        a.drag(target,40,0)
        point=a.point(target)
        a.page.mouse.move(point['x'],point['y'])
        a.page.keyboard.down('Shift');a.page.mouse.down()
        a.page.mouse.move(point['x']+40,point['y']);a.page.mouse.up();a.page.keyboard.up('Shift')
        a.dispatch('touchStart',[{**point,'x':point['x']-30},{**point,'id':2,'x':point['x']+30}])
        a.dispatch('touchMove',[{**point,'x':point['x']-50},{**point,'id':2,'x':point['x']+50}])
        a.dispatch('touchEnd',[])
        a.page.keyboard.down('Control');a.page.mouse.wheel(0,-120);a.page.keyboard.up('Control')
        a.page.wait_for_timeout(100)
        assert domains(a)==before
        a.page.locator('#disabled').uncheck()
    a.case('Plot', 'disabled viewport rejects touch and Shift drag', disabled)

    shape_hashes=set()
    for mode in range(7):
        def painted_pan(mode=mode):
            target=setup(a)
            a.page.locator('#plot-mode').select_option(str(mode))
            a.page.wait_for_timeout(100)
            before=raster(a,target)
            assert before not in shape_hashes, f'Initial mode {mode} reused earlier painted shape'
            shape_hashes.add(before)
            a.drag(target,30,10)
            after=raster(a,target)
            assert before!=after, f'Plot mode {mode} viewport changed without changing painted data'
        a.case('Plot', f'actual touch pan updates painted shape mode {mode}', painted_pan)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-plot-interactions.json')
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):
            pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            run(a)
            assert not a.errors,a.errors
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.write_text(json.dumps(a.results,indent=2)+'\n')
            a.page.screenshot(path=str(args.report.with_suffix('.png')),full_page=True)
            browser.close()
            failures=[case for case in a.results if case['status']!='verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} Plot interactions passed')
            if failures:
                raise SystemExit(1)
    finally:
        server.shutdown()


if __name__=='__main__':
    main()
