#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0", "pillow==11.3.0"]
# ///
"""Attachment card/group vertical paint containment, actual sizes and touch scrolling."""
import argparse,json,tempfile,threading
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from PIL import Image
from io import BytesIO
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
PAINT='''()=>{const e=cuiGallery.exports,d=new TextDecoder(),clips=[],out=[];for(let i=0;i<e.gallery_count();i++){
const type=e.gallery_command(i,0),b=[1,2,3,4].map(f=>e.gallery_command(i,f));
if(type===3){clips.push(b);continue;}if(type===4){clips.pop();continue;}
if(type===2)out.push({b,text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),clips:clips.map(c=>c.slice())});}return out;}'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=Path('build/web'));p.add_argument('--chromium');p.add_argument('--url',help='Optional published gallery URL');p.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-attachment-sizes.json');args=p.parse_args()
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright()as pw:
   browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}));a=Audit(browser,args.url.rstrip('/') if args.url else f'http://127.0.0.1:{server.server_port}');a.route('Attachment');a.page.set_viewport_size({'width':3000,'height':4000});a.page.evaluate("document.querySelector('main').style.maxWidth='none'")
   def containment():
    card=a.target('Attachment 1',9);x,y,w,h=card['f'][1:5]
    paints=a.page.evaluate(PAINT)
    matches=[d for d in paints if x<=d['b'][0]<x+w and y<=d['b'][1]<y+h and(d['text'].startswith('Attachment')or'Workspace'in d['text']or d['text']=='Open')]
    assert any('Workspace'in d['text']for d in matches),'Subtitle not painted'
    assert any(d['text']=='Open'for d in matches),'Open action not painted'
    for d in matches:
     b=d['b'];assert b[1]+b[3]<=y+h+.2,(d,card)
     assert all(b[1]>=c[1]-.2 and b[1]+b[3]<=c[1]+c[3]+.2 for c in d['clips']),('Vertical content cropped',d)
    assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   for density in [0,1]:
    a.configure('density',density)
    for vertical in [False,True]:
     a.configure('attachment-vertical',vertical)
     for size in range(1,15):
      a.configure('design-size',size)
      a.case('Attachment',f'design {size}, density {density}, vertical {vertical}: visible title/subtitle/action fit',containment)
   a.page.evaluate("document.querySelector('main').style.maxWidth=''");a.page.set_viewport_size({'width':1280,'height':900});a.configure('design-size',3);a.configure('attachment-vertical',False);a.configure('density',0)
   def pixels_and_scroll():
    containment();canvas=a.page.locator('canvas');canvas.scroll_into_view_if_needed()
    image=Image.open(BytesIO(canvas.screenshot())).convert('RGB')
    subtitle=next(d for d in a.page.evaluate(PAINT)if d['text']=='Workspace file')
    x,y,w,h=subtitle['b'];crop=image.crop((int(x),int(y),min(image.width,int(x+w+1)),int(y+h+1)))
    assert len(set(crop.getdata()))>3,'Subtitle visible pixels missing'
    owner=a.target(predicate=lambda s:s['f'][17]and s['f'][20]>40)
    before=owner['f'][18];a.drag(owner,-60,0,rx=.5,ry=.5)
    assert a.value(owner,18)>before,'Actual touch failed horizontal group scroll'
   a.case('Attachment','actual subtitle pixels and CDP horizontal pan',pixels_and_scroll)
   a.page.set_viewport_size({'width':390,'height':900})
   for density in [0,1]:
    a.configure('density',density)
    for size in [2,3]:
     a.configure('design-size',size)
     a.case('Attachment',f'mobile 390px design {size} density {density} subtitle/action containment',containment)
   a.page.locator('canvas').screenshot(path=str(args.report.with_suffix('.png')))
   a.page.set_viewport_size({'width':1280,'height':900});a.configure('density',0);a.configure('design-size',14)
   def reachable_action():
    owner=a.target(predicate=lambda s:s['f'][17]and s['f'][20]>40)
    button=a.target('Open');assert button['f'][1]>owner['f'][1]+owner['f'][3],'Expected initially offscreen large action'
    for _ in range(12):
     a.drag(owner,-400,0,rx=.8,ry=.5)
     button=a.target('Open')
     if button['f'][1]>=owner['f'][1]and button['f'][1]+button['f'][3]<=owner['f'][1]+owner['f'][3]:break
    else:raise AssertionError('Touch pan did not expose large Open action')
    before=a.page.evaluate('cuiGallery.exports.gallery_attachment_stats(3)')
    a.tap(button)
    assert a.page.evaluate('cuiGallery.exports.gallery_attachment_stats(3)')==before+1,'Exposed Open action did not activate'
   a.case('Attachment','largest card offscreen Open reachable by actual touch pan and tap',reachable_action)
   args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(a.results,indent=2)+'\n');assert not a.errors,a.errors
   failures=[r for r in a.results if r['status']!='verified'];print(f'{len(a.results)-len(failures)}/{len(a.results)} Attachment cases passed');browser.close()
   if failures:raise SystemExit(1)
 finally:server.shutdown()
if __name__=='__main__':main()
