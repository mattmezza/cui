#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0", "pillow==11.3.0"]
# ///
"""Actual CUI portrait pixels, overlapping Avatar controls and compact messages."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import tempfile
import threading
from PIL import Image
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

BITMAPS = '''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i)
 .filter(i=>e.gallery_command(i,0)===5&&e.gallery_bitmap_pixels(i)&&e.gallery_bitmap(i,0)===96&&e.gallery_bitmap(i,1)===96).map(i=>{
  const w=e.gallery_bitmap(i,0),h=e.gallery_bitmap(i,1),stride=e.gallery_bitmap(i,2);
  const p=new Uint8Array(e.memory.buffer,e.gallery_bitmap_pixels(i),stride*h);
  const at=(x,y)=>Array.from(p.slice(y*stride+x*4,y*stride+x*4+4));
  const colors=new Set();let edge=false;for(let y=0;y<h;y++)for(let x=0;x<w;x++){
   const c=at(x,y);if(c[3]===255)colors.add(c.slice(0,3).join(','));if(c[3]>0&&c[3]<255)edge=true;}
  return {i,w,h,pointer:e.gallery_bitmap_pixels(i),corners:[at(0,0),at(w-1,0),at(0,h-1),at(w-1,h-1)],
   center:at(w/2,h/2),colors:colors.size,edge,bounds:[1,2,3,4].map(f=>e.gallery_command(i,f))};});}'''


def run(a, directory):
    expected=[]
    for name in ['alex','jamie','robin']:
        with Image.open(directory/'assets'/f'avatar-{name}-96.png') as image:
            expected.append(list(image.convert('RGBA').getpixel((48,48))))

    def photos():
        bitmaps=a.page.evaluate(BITMAPS)
        assert len(bitmaps)==4, f'Expected three stack and one compact portrait: {len(bitmaps)}'
        for index,bitmap in enumerate(bitmaps[:3]):
            assert bitmap['w']==bitmap['h']==96
            assert bitmap['center']==expected[index], 'WASM pixels differ from committed portrait derivative'
            assert all(c[3]==0 for c in bitmap['corners']), 'Photo corners were not circularly masked'
            assert bitmap['edge'] and bitmap['colors']>500, 'Missing antialiasing or genuine image detail'
        assert len({b['pointer'] for b in bitmaps})==3, 'Cache was duplicated per appearance'
        assert a.page.locator('#preview-card img').count()==0, 'Portrait used DOM image stand-in'
        a.target('Sam Patel',20)
        assert 'SP' in a.texts(), 'Initials fallback disappeared'
    a.case('Avatar','genuine committed portrait pixels, circular alpha, shared cache and initials fallback',photos)

    def stack():
        team=a.target('Project team avatars',9)
        buttons=[a.target(name,1) for name in ['Alex Morgan','Jamie Chen','Robin Hayes']]
        assert team['f'][3]==156
        assert buttons[1]['f'][1]-buttons[0]['f'][1]==36
        assert buttons[2]['f'][1]-buttons[1]['f'][1]==36
        a.target('5 more people',20)
        compact=a.target('Compact team avatars',9)
        assert compact['f'][3]==70 and compact['f'][3]<=96
        a.target('7 more people',20)
        # This point lies in both Alex and Jamie bounds; later Jamie must win.
        point=a.point(buttons[1],.1,.5)
        a.dispatch('touchStart',[point]);a.dispatch('touchEnd',[])
        assert 'Selected profile: Jamie Chen' in a.texts()
        a.tap(a.target('Robin Hayes',1))
        assert 'Selected profile: Robin Hayes' in a.texts()
        a.drag(a.target('Alex Morgan',1),20,0)
        assert 'Selected profile: Robin Hayes' in a.texts(), 'Drag caused ghost profile activation'
    a.case('Avatar','responsive overlap, overflow count and real CDP topmost profile selection',stack)

    def disabled():
        a.page.locator('#disabled').check();a.page.wait_for_timeout(80)
        before=[text for text in a.texts() if text.startswith('Selected profile:')]
        target=a.target('Alex Morgan',1);assert target['f'][5]
        a.tap(target)
        assert [text for text in a.texts() if text.startswith('Selected profile:')]==before
        a.page.locator('#disabled').uncheck()
    a.case('Avatar','disabled profile controls preserve selection',disabled)

    def message():
        bitmaps=a.page.evaluate(BITMAPS)
        assert len(bitmaps)==2 and all(all(c[3]==0 for c in b['corners']) for b in bitmaps)
        assert 'Can you review the release notes?' in a.texts()
        assert "Looks good. I'll ship the update." in a.texts()
        assert 'Delivered · 09:30' in a.texts() and '09:32 · Read' in a.texts()
        assert 'A second message body' not in a.texts() and 'Workspace is ready.' not in a.texts()
        assert len([s for s in a.snapshot() if s['f'][0]==37])==2
        before=a.page.evaluate('cuiGallery.exports.gallery_message_actions()')
        a.tap(a.target('Like message',1))
        assert a.page.evaluate('cuiGallery.exports.gallery_message_actions()')==before+1
        assert 'Liked' in a.texts()
        checkbox=a.target('Acknowledge message',2)
        checked=checkbox['f'][6];a.tap(checkbox)
        assert a.value(checkbox,6)!=checked
    a.case('Message','compact incoming/outgoing portraits, timestamp/delivery and actual reaction callbacks',message)

    def message_disabled():
        a.page.locator('#disabled').check();a.page.wait_for_timeout(80)
        before=a.page.evaluate('cuiGallery.exports.gallery_message_actions()')
        target=a.target('Like message',1);assert target['f'][5]
        a.tap(target)
        assert a.page.evaluate('cuiGallery.exports.gallery_message_actions()')==before
        checkbox=a.target('Acknowledge message',2);checked=checkbox['f'][6];a.tap(checkbox)
        assert a.value(checkbox,6)==checked
        a.page.locator('#disabled').uncheck()
    a.case('Message','disabled footer actions reject touch updates',message_disabled)

    def variants():
        a.page.reload();a.page.wait_for_function('window.cuiGallery && cuiGallery.renderedComponent');a.route('Message')
        a.configure('message-avatar',True)
        a.configure('disabled',False)
        for alignment in ['0','1']:
            a.page.locator('#message-alignment').select_option(alignment)
            for variant in map(str,range(7)):
                a.page.locator('#message-variant').select_option(variant);a.page.wait_for_timeout(60)
                assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
                assert len(a.page.evaluate(BITMAPS))==2, f'Avatar images lost at alignment {alignment}, variant {variant}: {len(a.page.evaluate(BITMAPS))}, {a.texts()}'
                assert len([s for s in a.snapshot() if s['f'][0]==37])==2
        a.configure('message-avatar',False);a.page.wait_for_timeout(150)
        assert not a.page.evaluate(BITMAPS), f'Unchecking Avatar retained photo draw commands: {a.page.locator("#message-avatar").is_checked()}, {a.page.evaluate(BITMAPS)}'
        a.page.locator('#message-avatar').check()
    a.case('Message','both alignments, seven bubble selectors and optional avatar stay coherent',variants)

    def scroller():
        a.page.reload();a.page.wait_for_function('window.cuiGallery && cuiGallery.renderedComponent');a.route('MessageScroller')
        assert a.page.evaluate('cuiGallery.exports.gallery_conversation(0)')==80
        assert 0<a.page.evaluate('cuiGallery.exports.gallery_conversation(7)')<32
        assert a.page.evaluate(BITMAPS), 'Visible conversation rows have no real portraits'
        assert '09:32 · Read' in a.texts()
        owner=a.target('Conversation',38)
        anchor=a.target(role=37,predicate=lambda s:owner['f'][2]+30<s['f'][2]<owner['f'][2]+150)
        offset=owner['f'][19]
        start=a.point(owner,.1,.3)
        a.dispatch('touchStart',[start])
        # Reveal and measure earlier rows, then verify the same contact's next delta.
        a.dispatch('touchMove',[{**start,'y':start['y']+60}])
        anchor=a.target(anchor['name'],37)
        offset=a.target('Conversation',38)['f'][19]
        a.dispatch('touchMove',[{**start,'y':start['y']+90}])
        a.dispatch('touchEnd',[])
        after=a.target(anchor['name'],37)
        assert abs(after['f'][2]-anchor['f'][2]-30)<.2, f'Measured visible row failed exact touch pan: {anchor["f"][2]} -> {after["f"][2]}, offset {offset} -> {a.target("Conversation",38)["f"][19]}'
        assert a.target('Conversation',38)['f'][19]<offset, 'Touch pan did not reduce virtual offset'
        before=a.page.evaluate('cuiGallery.exports.gallery_conversation(0)')
        a.tap(a.target('Add message',1))
        assert a.page.evaluate('cuiGallery.exports.gallery_conversation(0)')==before+1
    a.case('MessageScroller','virtual conversation uses portraits/delivery, touch scrolling and append model',scroller)

    def mobile(family):
        a.page.set_viewport_size({'width':390,'height':844});a.route(family)
        snapshot=a.snapshot();canvas=a.page.locator('canvas').bounding_box()
        assert all(s['f'][1]>=0 and s['f'][1]+s['f'][3]<=canvas['width']+.2 for s in snapshot), 'Semantic row spills mobile canvas'
        if family=='Avatar':
            assert a.target('Project team avatars',9)['f'][3]<=canvas['width']-48
        else:
            text_bounds=a.page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===2&&d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))).startsWith('Can you review')||d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==='notes?').map(i=>[1,2,3,4].map(f=>e.gallery_command(i,f)));}''')
            assert text_bounds and all(b[2]<=280 and b[0]+b[2]<=canvas['width'] for b in text_bounds), (text_bounds,canvas)
        assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
        a.page.set_viewport_size({'width':1280,'height':900})
    for family in ['Avatar','Message']:
        a.case(family,'390px compact layout remains inside canvas and page',lambda family=family:mobile(family))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium')
    parser.add_argument('--report',type=Path,default=Path(tempfile.gettempdir())/'cui-avatar-message.json')
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            run(a,args.directory)
            assert not a.errors,a.errors
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.write_text(json.dumps(a.results,indent=2)+'\n')
            for family in ['Avatar','Message']:
                a.route(family);a.page.screenshot(path=str(args.report.with_name(f'avatar-message-{family}.png')),full_page=True)
            browser.close()
            failures=[case for case in a.results if case['status']!='verified']
            print(f'{len(a.results)-len(failures)}/{len(a.results)} Avatar/Message cases passed')
            if failures: raise SystemExit(1)
    finally:
        server.shutdown()


if __name__=='__main__': main()
