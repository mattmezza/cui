# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM scheduling/pixel/semantics checks in pinned Chromium 1208."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page(viewport={'width':1200,'height':900})
            errors=[]; page.on('pageerror',lambda e:errors.append(str(e)))
            for family in ['Spinner','Skeleton']:
                page.goto(f'http://127.0.0.1:{server.server_port}/#{family}')
                page.wait_for_function('(name)=>window.cuiGallery?.renderedComponent===name',arg=family)
                page.wait_for_timeout(80)
                assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
                assert page.evaluate('cuiGallery.exports.gallery_next_wake()')>0
                # Inspect actual canvas pixels as well as the SVG mask/rectangle.
                assert page.evaluate('''()=>{const c=document.querySelector('canvas'),d=c.getContext('2d').getImageData(0,0,c.width,c.height).data;return d.some((v,i)=>i%4!==3&&v>0)}''')
                moving_pixels=page.locator('canvas').screenshot()
                before=page.evaluate('cuiGallery.frame')
                page.wait_for_function('(before)=>cuiGallery.frame>before+2',arg=before)
                page.wait_for_timeout(200)
                assert page.locator('canvas').screenshot()!=moving_pixels
                if family=='Spinner':
                    assert page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>{if(e.gallery_command(i,0)!==5)return false;const w=e.gallery_bitmap(i,0),h=e.gallery_bitmap(i,1),stride=e.gallery_bitmap(i,2);const pixels=new Uint8Array(e.memory.buffer,e.gallery_bitmap_pixels(i),stride*h);return w>0&&h>0&&pixels.some((v,j)=>j%4===3&&v>0);});}''')
                assert page.locator('#accessible [role=status],#accessible [aria-busy=true]').count()==0
                page.emulate_media(reduced_motion='reduce')
                page.wait_for_function('cuiGallery.renderedReducedMotion===true')
                page.wait_for_timeout(100)
                assert page.evaluate('cuiGallery.exports.gallery_next_wake()')==0
                assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
                idle=page.evaluate('cuiGallery.frame')
                screenshot=page.locator('canvas').screenshot()
                page.wait_for_timeout(180)
                assert page.evaluate('cuiGallery.frame')==idle
                assert page.locator('canvas').screenshot()==screenshot
                page.emulate_media(reduced_motion='no-preference')
            # Preserve existing Progress indeterminate/reduced-motion semantics.
            page.goto(f'http://127.0.0.1:{server.server_port}/#Progress')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Progress"')
            page.locator('#progress-mode').select_option('1')
            page.wait_for_function('document.querySelector("#accessible [role=progressbar][aria-busy=true]")!==null')
            before=page.evaluate('cuiGallery.frame')
            page.wait_for_function('(before)=>cuiGallery.frame>before+2',arg=before)
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.renderedReducedMotion===true')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
            assert not errors,errors
            browser.close()
    finally: server.shutdown()
    print('Spinner/Skeleton finite wake + reduced-motion quiescence; Progress semantics PASS')
if __name__=='__main__': main()
