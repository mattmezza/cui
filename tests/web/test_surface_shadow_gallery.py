#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual gallery opt-in Popover and Notification consume their copied source shadows."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
SHADOWS = '''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===7).map(i=>Array.from({length:16},(_,f)=>e.gallery_shadow_value(i,f)));}'''
def main():
    p=argparse.ArgumentParser();p.add_argument('--directory',type=Path,default=Path('build/web'));a=p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*_args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(a.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            b=pw.chromium.launch(headless=True)
            for dpr in [1,2]:
                for width in [1280,390]:
                    page=b.new_page(viewport={'width':width,'height':1000},device_scale_factor=dpr)
                    errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
                    page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
                    page.wait_for_function('window.cuiGallery&&cuiGallery.renderedComponent==="Button"')
                    audit=Audit.__new__(Audit); audit.page=page; audit.errors=errors; page.emulate_media(reduced_motion='reduce')
                    for name,trigger,geometry in [
                        ('Popover','Open account card',[[0,0,0,1],[0,4,3,-1],[0,2,2,-2]]),
                        ('Notification','Add notification',[[0,10,7.5,-3],[0,4,3,-4]])]:
                        audit.route(name)
                        point=audit.point(audit.target(trigger))
                        page.mouse.click(point['x'],point['y'])
                        page.wait_for_function(f'({SHADOWS})().length==={len(geometry)}')
                        values=page.evaluate(SHADOWS)
                        assert [v[8:12] for v in values]==geometry,(name,values)
                        assert all(v[15]==26 and v[:8]==values[0][:8] for v in values),(name,values)
                        assert not errors and page.evaluate('cuiGallery.exports.gallery_error()')==0,(name,errors)
                        assert values[0][2]>0 and values[0][3]>0
                        print(f'PASS actual {name} copied caster/layers: width={width}, DPR={dpr}',flush=True)
                    page.close()
            b.close()
    finally: server.shutdown()
if __name__=='__main__': main()
