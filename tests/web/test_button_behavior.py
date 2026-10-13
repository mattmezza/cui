#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Button callbacks, inert loading, icon slots and pointer lifecycle."""
import argparse,json,tempfile,threading
from functools import partial
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=Path('build/web'));p.add_argument('--chromium');p.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-button-behavior.json');args=p.parse_args()
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright()as pw:
   browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}));a=Audit(browser,f'http://127.0.0.1:{server.server_port}');a.route('Button')
   def clicks():return a.page.evaluate('cuiGallery.exports.gallery_clicks()')
   def target():return a.target('Run action',1)
   def activation():
    n=clicks();a.tap(target());assert clicks()==n+1,'Touch callback count';a.page.wait_for_timeout(400);assert clicks()==n+1,'Ghost mouse duplicated touch callback'
    pt=a.point(target());a.page.mouse.click(pt['x'],pt['y']);assert clicks()==n+2,'Mouse callback count'
    assert not target()['f'][7],'Opt-in pointer focus unexpectedly changed'
    a.page.keyboard.press('Tab');a.page.keyboard.press('Enter');a.page.wait_for_timeout(60);assert clicks()==n+3,'Tab/Enter activation missing'
   a.case('Button','touch and mouse exact-once callbacks; pointer focus opt-out preserves keyboard',activation)
   def cancelled():
    n=clicks();pt=a.point(target());a.dispatch('touchStart',[pt]);a.dispatch('touchCancel',[]);assert clicks()==n
    a.drag(target(),60,0);assert clicks()==n,'Touch drag activated';a.tap(target());assert clicks()==n+1,'Next tap after cancellation failed'
   a.case('Button','cancel/drag suppression and next touch tap',cancelled)
   def paint_states():
    a.page.mouse.move(5,5);a.page.wait_for_timeout(70)
    def fill():
     b=target()['f'][1:5]
     return a.page.evaluate("""b=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===0&&[1,2,3,4].every((f,j)=>Math.abs(e.gallery_command(i,f)-b[j])<.5))return [5,6,7,8].map(f=>e.gallery_command(i,f));return null;}""",b)
    normal=fill();pt=a.point(target());a.page.mouse.move(pt['x'],pt['y']);a.page.wait_for_timeout(70)
    hovered=fill();assert normal and hovered and normal!=hovered,'Hover palette did not change'
    assert a.page.locator('canvas').evaluate("e=>getComputedStyle(e).cursor")=='pointer','Custom hand cursor not applied'
    a.page.mouse.down();a.page.wait_for_timeout(70);pressed=fill();assert pressed!=hovered,'Pressed palette did not change';a.page.mouse.up()
   a.case('Button','actual hover/pressed palettes and custom hand cursor',paint_states)
   def disabled():
    n=clicks();a.configure('disabled',True);a.tap(target());assert clicks()==n;a.configure('disabled',False)
    loading=a.target('Loading',1);assert loading['f'][5] and loading['f'][14],'Loading not disabled/busy';a.tap(loading);assert clicks()==n
   a.case('Button','disabled and loading remain inert',disabled)
   def slots():
    t=a.target('With icon',1);x,y,w,h=t['f'][1:5]
    glyphs=a.page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),b:[1,2,3,4].map(f=>e.gallery_command(i,f))})).filter(c=>c.type===5);}''')
    actual=[g for g in glyphs if y<=g['b'][1]<y+h];assert len(actual)==3,('Expected leading, trailing, caret SVGs',actual)
    assert all(g['b'][0]>=x-.5 and g['b'][0]+g['b'][2]<=x+w+.5 and g['b'][1]+g['b'][3]<=y+h+.5 for g in actual),'Icon escaped button'
    assert 'NEW' in a.texts(),'Appended child missing';full=a.target('A long accessible action label with bounded ellipsis',1);assert full
    assert any('…'in text for text in a.texts()),'Bounded ellipsis missing'
   a.case('Button','real leading/trailing/caret SVGs and appended content/accessible ellipsis',slots)
   def variant(name):
    t=a.target(name,1);assert t['f'][8]==0 and not t['f'][6],('Presentation altered selection semantics',t)
   for name in ['Default','Warning','Success','Info','Text','Custom']:
    a.case('Button',f'{name} variant exists with stable semantics',lambda name=name: variant(name))
   args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(a.results,indent=2)+'\n');fail=[r for r in a.results if r['status']!='verified'];print(f'{len(a.results)-len(fail)}/{len(a.results)} Button cases passed');browser.close()
   if fail or a.errors:raise SystemExit(1)
 finally:server.shutdown()
if __name__=='__main__':main()
