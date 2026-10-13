#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Table naming/sizing and DataTable selection-only actions/sort cycle."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
PAINT='''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),b:[1,2,3,4].map(f=>e.gallery_command(i,f)),text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))}));}'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=Path('build/web'));p.add_argument('--chromium');args=p.parse_args()
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright()as pw:
   browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}));a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
   a.route('Table')
   heights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264]
   for density in [0,1]:
    a.configure('density',density)
    for size in range(1,15):
     a.configure('design-size',size)
     root=a.target('Recent workspace files',26);assert root['f'][24:26]==[4,3]
     header=a.target('Name',29);assert header['f'][22:24]==[1,1]and header['f'][26]
     assert abs(header['f'][4]-heights[size]*(.875 if density else 1))<1,(density,size,header)
     cells=[s for s in a.snapshot()if s['f'][0]==28];assert len(cells)==9
     assert all(s['name']!='Cell'and s['f'][22]>1 and s['f'][23]>0 for s in cells)
     paints=a.page.evaluate(PAINT)
     text=next(c for c in paints if c['text']=='Name')
     assert text['b'][1]>=header['f'][2]and text['b'][1]+text['b'][3]<=header['f'][2]+header['f'][4]+.5
     assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   a.configure('density',0);a.route('DataTable');a.configure('table-selection',2)
   a.target('Workspace files',31)
   header=a.target('Name',29)
   for sort in [2,1,0]:
    a.tap(a.target('Name',29));assert a.value(header,8)==sort
   # Selection opt-out retains base action behavior and rejects row/cell selection.
   a.tap(a.target('Actions off',1));a.tap(a.target('Select on',1))
   before=[s['id']for s in a.snapshot()if s['f'][0]==30 and s['f'][11]]
   a.tap(a.target('Inspect',1))
   assert any('inspections: 1'in t for t in a.texts())
   after=[s['id']for s in a.snapshot()if s['f'][0]==30 and s['f'][11]];assert after==before
   a.tap(a.target('Select off',1))
   cell=a.target('Ready',30);assert cell['f'][26]and not cell['f'][5]
   a.tap(cell);assert a.value(cell,11)==0
   a.tap(a.target('Status',29));assert a.target('Status',29)['f'][8]==2
   a.page.set_viewport_size({'width':390,'height':1200});a.route('DataTable')
   a.tap(a.target('Inspect',1));assert any('inspections: 3'in t for t in a.texts()),a.texts()
   a.configure('disabled',True);assert a.target('Inspect',1)['f'][5]
   a.configure('disabled',False)
   a.route('Table');a.configure('design-size',14)
   owner=a.target(predicate=lambda s:s['f'][17]and s['f'][20]>40)
   before=a.value(owner,18);a.drag(owner,-80,0,rx=.5,ry=.5)
   assert a.value(owner,18)>before
   assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   assert not a.errors,a.errors;browser.close()
 finally:server.shutdown()
 print('Table selection contracts browser passed')
if __name__=='__main__':main()
