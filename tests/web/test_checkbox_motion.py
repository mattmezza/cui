#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Strict actual WASM Checkbox spring, rich label, tooltip and CDP lifecycle checks."""
import argparse,json,tempfile,threading
from pathlib import Path
from functools import partial
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
MARK='''()=>{const e=cuiGallery.exports;let alpha=0;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===5)alpha=Math.max(alpha,e.gallery_bitmap(i,13));return alpha;}'''
PAINT='''()=>{const e=cuiGallery.exports,d=new TextDecoder(),clips=[],out=[];for(let i=0;i<e.gallery_count();i++){const t=e.gallery_command(i,0),b=[1,2,3,4].map(f=>e.gallery_command(i,f));if(t===3)clips.push(b);else if(t===4)clips.pop();else if(t===2)out.push({b,text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),clips:clips.map(c=>c.slice())});}return out;}'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=Path('build/web'));p.add_argument('--chromium');p.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-checkbox-motion.json');args=p.parse_args()
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright()as pw:
   browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}));a=Audit(browser,f'http://127.0.0.1:{server.server_port}');a.route('Checkbox');a.page.emulate_media(reduced_motion='no-preference')
   def target():return a.target('Receive updates',2)
   def point():
    a.page.locator('canvas').evaluate("e=>e.scrollIntoView({block:'center'})")
    t=target();r=a.page.locator('canvas').bounding_box();x=r['x']+t['f'][1]+5;y=r['y']+t['f'][2]+5
    top=a.page.locator('header').bounding_box()['height']+8
    if y<top:a.page.evaluate('(d)=>scrollBy(0,d)',y-top);r=a.page.locator('canvas').bounding_box();y=r['y']+t['f'][2]+5
    assert a.page.evaluate('p=>document.elementFromPoint(p.x,p.y)===document.querySelector("canvas")',{'x':x,'y':y}),'Indicator point occluded'
    return {'id':91,'x':x,'y':y}
   def tap():a.dispatch('touchStart',[point()]);a.dispatch('touchEnd',[])
   def settle():a.page.wait_for_timeout(900)
   def motion():
    before=target()['f'][6]
    a.page.evaluate('''()=>{window.checkboxSamples=[];window.checkboxSampling=true;function sample(){const e=cuiGallery.exports;let alpha=0;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===5)alpha=Math.max(alpha,e.gallery_bitmap(i,13));checkboxSamples.push({alpha,checked:e.gallery_checkbox_state(-1),wake:e.gallery_next_wake()});if(checkboxSampling)requestAnimationFrame(sample);}sample();}''')
    tap();assert target()['f'][6]!=before,'Logical checked state did not change'
    settle();a.page.evaluate('checkboxSampling=false')
    samples=a.page.evaluate('checkboxSamples');assert any(0<s['alpha']<255 for s in samples),'No actual intermediate spring mark paint'
    assert a.page.evaluate(MARK)==255 and a.page.evaluate('cuiGallery.exports.gallery_next_wake()')==0,'Checked spring did not quiesce'
    tap();settle();assert target()['f'][6]==before and a.page.evaluate(MARK)==0,'Uncheck did not remove mark'
   a.case('Checkbox','actual touch updates semantics immediately and mark animates then quiesces',motion)
   def mixed():
    tap();settle()
    a.page.evaluate('''()=>{window.mixedSamples=[];window.mixedSampling=true;function sample(){const e=cuiGallery.exports;let alpha=0;for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===5)alpha=Math.max(alpha,e.gallery_bitmap(i,13));mixedSamples.push(alpha);if(mixedSampling)requestAnimationFrame(sample);}sample();}''')
    a.configure('checkbox-state',2)
    assert target()['f'][8]==2 and not target()['f'][6],'Mixed semantics did not update immediately'
    settle();a.page.evaluate('mixedSampling=false')
    assert any(0<alpha<255 for alpha in a.page.evaluate('mixedSamples')),'Checked mark did not fade during mixed transition'
    assert a.page.evaluate(MARK)==0,'Mixed state retained check glyph'
    tap();assert target()['f'][6],'Mixed touch did not request checked'
    settle();tap();settle()
   a.case('Checkbox','DOM mixed-state configuration crossfades mark; actual touch selects checked',mixed)
   def cancellation():
    before=target()['f'][6];a.dispatch('touchStart',[point()]);a.dispatch('touchCancel',[]);settle();assert target()['f'][6]==before and not any('Change email'in text for text in a.texts())
    start=point();a.dispatch('touchStart',[start]);a.dispatch('touchMove',[{**start,'x':start['x']+30}]);a.dispatch('touchMove',[start]);a.dispatch('touchEnd',[]);assert target()['f'][6]==before,'Backtracking touch drag activated'
    a.configure('disabled',True);tap();assert target()['f'][6]==before;a.configure('disabled',False)
   a.case('Checkbox','cancel and disabled touch preserve value',cancellation)
   def reduced():
    a.page.emulate_media(reduced_motion='reduce');tap();assert a.page.evaluate(MARK)==255
    a.page.keyboard.press('Escape');a.page.wait_for_timeout(80);assert a.page.evaluate('cuiGallery.exports.gallery_next_wake()')==0
    tap();assert a.page.evaluate(MARK)==0;a.page.emulate_media(reduced_motion='no-preference')
   a.case('Checkbox','reduced motion snaps checked and unchecked paint',reduced)
   def tooltip():
    point();r=a.page.locator('canvas').bounding_box();blank={'id':92,'x':r['x']+r['width']-10,'y':r['y']+r['height']-10};a.dispatch('touchStart',[blank]);a.dispatch('touchEnd',[])
    tap();settle();assert any('Change email'in text for text in a.texts()),'Touch focus tooltip missing'
    a.page.keyboard.press('Escape');a.page.wait_for_timeout(80);assert not any('Change email'in text for text in a.texts()),'Escape did not dismiss tooltip'
    before=target()['f'][6];a.page.keyboard.press('Space');a.page.wait_for_function('(before)=>cuiGallery.exports.gallery_checkbox_state(-1)!==before',arg=before);assert target()['f'][6]!=before,'Focused Space did not toggle'
    settle()
   a.case('Checkbox','touch focus opens tooltip; Escape dismisses; keyboard toggles',tooltip)
   def geometry():
    t=target();x,y,w,h=t['f'][1:5];paints=a.page.evaluate(PAINT)
    matches=[d for d in paints if y<=d['b'][1]<y+h and not d['text'].startswith('Change email')]
    assert matches and any('Email'in d['text']for d in matches),'Rich label detail missing'
    for d in matches:
     b=d['b'];assert b[0]>=x-.2 and b[0]+b[2]<=x+w+.2,('Label outside Checkbox width',d,t)
     assert b[1]>=y-.2 and b[1]+b[3]<=y+h+.2,('Label outside Checkbox',d,t)
     assert all(b[1]>=c[1]-.2 and b[1]+b[3]<=c[1]+c[3]+.2 for c in d['clips']),('Label vertically clipped',d)
    assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   for density in [0,1]:
    a.configure('density',density)
    for size in range(1,15):
     a.configure('design-size',size);a.case('Checkbox',f'wrapped rich label design {size} density {density} fits',geometry)
   a.page.set_viewport_size({'width':390,'height':900});a.configure('design-size',3);a.configure('density',0);a.case('Checkbox','mobile Base wrapped label fits actual clip',geometry)
   args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(a.results,indent=2)+'\n');assert not a.errors,a.errors
   failures=[r for r in a.results if r['status']!='verified'];print(f'{len(a.results)-len(failures)}/{len(a.results)} Checkbox cases passed');browser.close()
   if failures:raise SystemExit(1)
 finally:server.shutdown()
if __name__=='__main__':main()
