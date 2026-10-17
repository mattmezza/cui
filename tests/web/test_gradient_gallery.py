# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM typography paint controls, serialized stops and Canvas glyph pixels."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

PAINTS = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({i,type:e.gallery_command(i,0),
 rect:[1,2,3,4].map(f=>e.gallery_command(i,f)),kind:e.gallery_command(i,25),
 axis:[26,27,28,29].map(f=>e.gallery_command(i,f)),
 stops:Array.from({length:e.gallery_command(i,30)},(_,s)=>Array.from({length:5},(_,v)=>e.gallery_command(i,31+s*5+v))),
 text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))}));}'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium')
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*_args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            for width in [1280,390]:
                for dpr in [1,2]:
                    context=browser.new_context(viewport={'width':width,'height':900},device_scale_factor=dpr)
                    page=context.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
                    page.goto(f'http://127.0.0.1:{server.server_port}/#Theme')
                    page.wait_for_function('window.cuiGallery?.renderedComponent==="Theme"')
                    page.locator('#text-size').select_option('48')
                    page.locator('#text-paint').select_option('1')
                    page.wait_for_function('('+PAINTS+')().some(c=>c.kind===1)')
                    paints=[c for c in page.evaluate(PAINTS) if c['kind']==1]
                    assert paints and all(c['type']==2 for c in paints)
                    assert all(c['stops']==[[0,244,40,90,255],[1,30,90,250,255]] for c in paints)
                    assert all(c['axis'][2]>c['axis'][0] for c in paints)
                    # Read actual rendered interiors of the first glyph, not the
                    # background or a separately generated color preview.
                    result=page.evaluate('''c=>{const cv=document.querySelector('#preview'),ctx=cv.getContext('2d'),
                     scale=cv.width/cv.clientWidth,[x,y,w,h]=c.rect;
                     const left=Math.floor(x*scale),top=Math.floor(y*scale),iw=Math.ceil(40*scale),ih=Math.ceil(h*scale);
                     const pixels=ctx.getImageData(left,top,iw,ih).data;
                     const colors=[];for(let py=0;py<ih;py++)for(let px=0;px<iw;px++){
                      const at=(py*iw+px)*4,t=Math.max(0,Math.min(1,((left+px+.5)/scale-c.axis[0])/(c.axis[2]-c.axis[0])));
                      const expected=[244-214*t,40+50*t,90+160*t].map(Math.round);
                      // Only fully covered glyph interiors match the spatial
                      // field in all channels; antialias/background edges do not.
                      if(expected.every((v,k)=>Math.abs(pixels[at+k]-v)<=2))colors.push(pixels[at]);
                     }
                     return {count:colors.length,range:Math.max(...colors)-Math.min(...colors)};
                    }''',paints[0])
                    assert result['count']>10 and result['range']>3,(width,dpr,result,paints)
                    assert 'cui_text_paragraph_painted' in page.locator('#snippet').inner_text()
                    page.evaluate('''()=>{window.copiedCode=null;Object.defineProperty(navigator,'clipboard',{configurable:true,value:{writeText:async text=>{window.copiedCode=text;}}});}''')
                    page.locator('#copy-snippet').click()
                    page.wait_for_function('window.copiedCode?.includes("cui_text_gradient_stop")')
                    page.locator('#text-paint').select_option('0')
                    page.wait_for_function('!('+PAINTS+')().some(c=>c.kind===1)')
                    assert page.evaluate('cuiGallery.exports.gallery_error()')==0
                    page.locator('#component-guides').get_by_role('link',name='Gradient text',exact=True).click()
                    page.locator('#docs-title').filter(has_text='Gradient text').wait_for(state='visible')
                    assert not errors,errors
                    context.close()
            browser.close()
        print('PASS real WASM gradient showcase, glyph pixels, copied API and guide: desktop/390, DPR 1/2')
    finally:server.shutdown();server.server_close()
if __name__=='__main__':main()
