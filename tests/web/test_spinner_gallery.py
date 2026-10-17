# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Spinner controls, SVG pixels, reduced-motion quiescence and usage links."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

IMAGE = '''()=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===5){
 return {rect:[1,2,3,4].map(f=>e.gallery_command(i,f)),tint:[10,11,12,13].map(f=>e.gallery_bitmap(i,f)),
 mask:Array.from(new Uint8Array(e.memory.buffer,e.gallery_bitmap_pixels(i),e.gallery_bitmap(i,2)*e.gallery_bitmap(i,1)))};}return null;}'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*_args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,executable_path=args.chromium)
            for width in [1280,390]:
                for dpr in [1,2]:
                    context=browser.new_context(viewport={'width':width,'height':900},device_scale_factor=dpr,reduced_motion='reduce')
                    page=context.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
                    page.goto(f'http://127.0.0.1:{server.server_port}/#Spinner')
                    page.wait_for_function('window.cuiGallery?.renderedComponent==="Spinner"')
                    assert page.locator('#spinner-size').input_value()=='16'
                    assert page.locator('#spinner-icon').is_visible()
                    for size in [12,14,16,24,64]:
                        page.locator('#spinner-size').select_option(str(size))
                        page.wait_for_function('('+IMAGE+')()?.rect[2]==='+str(size))
                        image=page.evaluate(IMAGE)
                        assert image['rect'][3]==size and any(image['mask'][3::4])
                    page.locator('#spinner-color').select_option('1')
                    page.wait_for_function('('+IMAGE+')()?.tint[0]===220')
                    assert page.evaluate(IMAGE)['tint']==[220,40,90,255]
                    masks=[];shots=[]
                    for icon in range(3):
                        page.locator('#spinner-icon').select_option(str(icon))
                        page.wait_for_timeout(70)
                        masks.append(page.evaluate(IMAGE)['mask'])
                        shots.append(page.locator('canvas').screenshot())
                    assert len({bytes(m) for m in masks})==3
                    assert len(set(shots))==3
                    assert page.evaluate('cuiGallery.exports.gallery_next_wake()')==0
                    idle=page.evaluate('cuiGallery.frame');shot=page.locator('canvas').screenshot()
                    page.wait_for_timeout(100)
                    assert page.evaluate('cuiGallery.frame')==idle and page.locator('canvas').screenshot()==shot
                    page.locator('#spinner-color').select_option('2')
                    page.wait_for_function('('+IMAGE+')()?.tint[3]===0')
                    transparent=page.locator('canvas').screenshot()
                    assert transparent!=shot
                    page.locator('#spinner-color').select_option('1')
                    page.locator('#spinner-easing').select_option('1')
                    page.emulate_media(reduced_motion='no-preference')
                    page.wait_for_function('cuiGallery.renderedReducedMotion===false')
                    moving=page.locator('canvas').screenshot();frame=page.evaluate('cuiGallery.frame')
                    page.wait_for_function('(f)=>cuiGallery.frame>f+3',arg=frame)
                    assert page.locator('canvas').screenshot()!=moving
                    assert page.evaluate('cuiGallery.exports.gallery_next_wake()')>0
                    assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
                    assert page.locator('#accessible [role=status],#accessible [aria-busy=true]').count()==0
                    assert page.evaluate('cuiGallery.exports.gallery_error()')==0
                    page.evaluate('''()=>Object.defineProperty(navigator,'clipboard',{configurable:true,value:{writeText:async t=>window.copiedSpinner=t}})''')
                    page.locator('#copy-snippet').click()
                    page.wait_for_function('window.copiedSpinner?.includes("cui_spinner_ex")')
                    page.locator('#component-guides').get_by_role('link',name='Spinner',exact=True).click()
                    page.locator('#docs-title').filter(has_text='Spinner').wait_for(state='visible')
                    assert not errors,errors
                    context.close()
            browser.close()
        print('PASS Spinner sizes/icons/colors/easing, SVG pixels, motion/idle, copy/guide: desktop/390 DPR1/2')
    finally:server.shutdown();server.server_close()
if __name__=='__main__':main()
