#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Strict actual-pane geometry assertions using visible-divider mouse/CDP touch."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

def panes(a):
    return [a.target(name) for name in ('Explorer panel', 'Editor panel')]

def drag(a, touch, vertical, delta):
    start = a.point(a.target('Resize panels', 40))
    dx, dy = (0, delta) if vertical else (delta, 0)
    if touch:
        a.dispatch('touchStart', [start])
        for n in range(1, 7):
            a.dispatch('touchMove', [{**start, 'x':start['x']+dx*n/6, 'y':start['y']+dy*n/6}])
        a.dispatch('touchEnd', [])
    else:
        a.page.mouse.move(start['x'], start['y'])
        a.page.mouse.down()
        a.page.mouse.move(start['x']+dx, start['y']+dy, steps=6)
        a.page.mouse.up()
        a.page.wait_for_timeout(120)

def run(browser, url, output):
    results=[]
    for width in (390, 1280, 1920):
        for vertical in (False, True):
            for compact in (False, True):
                for touch in (False, True):
                    a=Audit(browser,url)
                    a.page.set_viewport_size({'width':width,'height':1080})
                    a.route('Resizable')
                    a.configure('resizable-vertical',vertical)
                    a.configure('density',int(compact))
                    before=panes(a)
                    content_before=a.target('main.c',1)['f'][3]
                    if width==1920 and not vertical and not compact:
                        a.page.screenshot(path=str(output/f'before-{touch}.png'))
                    drag(a,touch,vertical,18 if vertical else 36)
                    after=panes(a)
                    if width==1920 and not vertical and not compact:
                        a.page.screenshot(path=str(output/f'after-{touch}.png'))
                    field=4 if vertical else 3
                    delta=after[0]['f'][field]-before[0]['f'][field]
                    assert delta>12, f'{width=} {vertical=} {compact=} {touch=}: pane geometry did not resize ({delta})'
                    assert abs(after[1]['f'][field]-before[1]['f'][field]+delta)<1
                    position=2 if vertical else 1
                    assert abs(after[1]['f'][position]-before[1]['f'][position]-delta)<1
                    if not vertical:
                        assert abs(a.target('main.c',1)['f'][3]-content_before-delta)<1, 'Explorer content did not follow panel'
                    a.configure('resizable-disabled',True)
                    locked=panes(a)
                    drag(a,touch,vertical,-18)
                    assert [p['f'][1:5] for p in panes(a)]==[p['f'][1:5] for p in locked], 'Disabled divider moved'
                    a.configure('resizable-disabled',False)
                    handle=a.target('Resize panels',40)
                    # Focus through a real click, then exercise the documented keyboard fallback.
                    if touch: a.tap(handle)
                    else:
                        point=a.point(handle);a.page.mouse.click(point['x'],point['y'])
                    key_before=panes(a)[0]['f'][field]
                    a.page.keyboard.press('ArrowUp' if vertical else 'ArrowLeft')
                    a.page.wait_for_timeout(100)
                    assert panes(a)[0]['f'][field]<key_before-5, 'Keyboard resize failed'
                    assert not a.errors,a.errors
                    results.append({'width':width,'vertical':vertical,'compact':compact,'touch':touch,
                        'before':[p['f'][1:5] for p in before],'after':[p['f'][1:5] for p in after]})
                    print(f'PASS {width}px {"vertical" if vertical else "horizontal"} {"compact" if compact else "comfortable"} {"touch" if touch else "mouse"}', flush=True)
                    a.page.close()
    return results

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--url',default='https://matteo.merola.co/cui/')
    p.add_argument('--directory',type=Path)
    p.add_argument('--chromium')
    p.add_argument('--output',type=Path,default=Path('/tmp/cui-resizable-audit'))
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    server=None
    if args.directory:
        class Handler(SimpleHTTPRequestHandler):
            def log_message(self,*args): pass
        server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
        threading.Thread(target=server.serve_forever,daemon=True).start()
        args.url=f'http://127.0.0.1:{server.server_port}'
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            results=run(browser,args.url,args.output)
            (args.output/'report.json').write_text(json.dumps(results,indent=2)+'\n')
            browser.close()
            print(f'{len(results)}/24 actual Resizable geometry cases passed')
    finally:
        if server: server.shutdown()
if __name__=='__main__': main()
