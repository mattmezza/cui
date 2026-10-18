#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Bounded real Chromium decode/CUI routing proof, not a cross-browser claim."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
import hashlib
import json
import sys
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', type=Path)
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--chromium')
    args = parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    sys.path.insert(0,str(root/'scripts'))
    from build_web import public_version, versioned_index, SOURCES
    version=public_version((root/'include/cui/cui.h').read_text())
    for invalid in ['', '#define CUI_VERSION 1', '#define CUI_VERSION "a"\n#define CUI_VERSION "b"', '#define CUI_VERSION "a"\n#define CUI_VERSION invalid']:
        try: public_version(invalid)
        except ValueError: pass
        else: raise AssertionError(invalid)
    assert public_version('#define CUI_VERSION "0.1.0-alpha.2"')=='0.1.0-alpha.2'
    assert versioned_index('<span>__CUI_VERSION__</span>','#define CUI_VERSION "<&>"')=='<span>&lt;&amp;&gt;</span>'
    for invalid in ['', '__CUI_VERSION__ __CUI_VERSION__']:
        try: versioned_index(invalid,'#define CUI_VERSION "a"')
        except ValueError: pass
        else: raise AssertionError(invalid)
    for unit in ['cui_otp_events.c','cui_number_events.c','cui_numeric_selection_events.c','cui_tabs_events.c']:
        assert 'src/base/'+unit in SOURCES
    receipt=json.loads((root/'web/assets/video/receipt.json').read_text())
    for name,entry in receipt['files'].items():
        data=(root/'web/assets/video'/name).read_bytes()
        assert len(data)==entry['bytes'] and hashlib.sha256(data).hexdigest()==entry['sha256']
        assert len(data)<500000
    if args.offline:
        print('PASS: public version validation, required WASM sources, encoded asset hashes/bounds')
        return
    if not args.directory: parser.error('--directory required for browser proof')
    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            return super().translate_path(path[4:] if path.startswith('/cui/') else path)
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width':1280,'height':1000}, device_scale_factor=2)
            errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#Video')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Video" && cuiGallery.video.snapshot().frames>0')
            assert page.evaluate('cuiGallery.catalog.components.length')==78
            assert page.evaluate('cuiGallery.catalog.components.every(c=>c.name!=="Video")')
            assert page.evaluate('cuiGallery.catalog.extensions.map(c=>c.name)')==['Video']
            assert len(page.evaluate('cuiGallery.catalog.extensions[0].source_commit'))==40
            page.locator('#search').fill('video')
            assert page.locator('#catalog a[href="#Video"]').count()==1
            page.locator('#search').fill('')
            assert page.locator('.version').inner_text()==version
            assert 'cui_video_player' in page.locator('#snippet').inner_text()
            assert page.locator('#component-guides a').count()>=1
            assert page.locator('video').count()==0, 'decoder must not be a visible HTML overlay'
            def snap(): return page.evaluate('cuiGallery.video.snapshot()')
            def surface_hash(): return page.evaluate("""()=>{const c=document.querySelector('#preview'),d=c.width/c.clientWidth;
                const bytes=c.getContext('2d').getImageData(24*d,24*d,160*d,90*d).data;
                let hash=2166136261;for(let i=0;i<bytes.length;i+=97)hash=Math.imul(hash^bytes[i],16777619);return hash>>>0;}""")
            first=snap();first_surface=surface_hash()
            assert first['muted'] and first['loop'] and first['paused']
            assert first['duration']>=11
            assert first['source'].startswith('blob:')
            def action(label):
                page.locator('#accessible').get_by_role('button',name=label,exact=True).evaluate('(e)=>e.click()')
            # Actual canvas hit region, not an HTML media overlay.
            play=page.evaluate("""()=>{const w=cuiGallery.exports,decode=new TextDecoder();for(let i=0;i<w.gallery_semantic_count();i++){
                const name=decode.decode(new Uint8Array(w.memory.buffer,w.gallery_semantic_name(i),w.gallery_semantic_length(i)));
                if(name==='Play')return {x:w.gallery_semantic(i,1)+22,y:w.gallery_semantic(i,2)+22};}}""")
            bounds=page.locator('#preview').bounding_box()
            page.mouse.click(bounds['x']+play['x'],bounds['y']+play['y'])
            page.wait_for_function('!cuiGallery.video.snapshot().paused && cuiGallery.video.snapshot().time>.5')
            page.wait_for_function('(hash)=>cuiGallery.video.snapshot().checksum!==hash',arg=first['checksum'])
            assert surface_hash()!=first_surface, 'actual CUI surface pixels must change'
            action('Pause')
            page.wait_for_function('cuiGallery.video.snapshot().paused')
            paused=snap();page.wait_for_timeout(350)
            assert abs(snap()['time']-paused['time'])<.05
            before=snap()['time'];action('Skip forward')
            page.wait_for_function('(time)=>!cuiGallery.video.snapshot().seeking && cuiGallery.video.snapshot().time>time+1.8',arg=before)
            assert snap()['generation']>first['generation']
            action('Playback rate');page.wait_for_function('cuiGallery.video.snapshot().rate===1.5')
            # Genuine keyboard input is delivered through the existing canvas route.
            slider=page.evaluate('''()=>{let w=cuiGallery.exports;for(let i=0;i<w.gallery_semantic_count();i++)if(w.gallery_semantic(i,0)===5)return {i,x:w.gallery_semantic(i,1),y:w.gallery_semantic(i,2),width:w.gallery_semantic(i,3),height:w.gallery_semantic(i,4)};}''')
            page.evaluate('(i)=>{cuiGallery.exports.gallery_focus(i);cuiGallery.render();}',slider['i'])
            page.locator('#preview').focus()
            before=snap()['time'];page.keyboard.press('ArrowRight')
            page.wait_for_function('(time)=>cuiGallery.video.snapshot().time>time+.5',arg=before)
            assert any(r['kind']==1 for r in snap()['requests'])
            # A real pointer scrub cancellation restores its begin position.
            page.wait_for_function('!cuiGallery.video.snapshot().seeking')
            origin=snap()['time'];bounds=page.locator('#preview').bounding_box()
            x=bounds['x']+slider['x'];y=bounds['y']+slider['y']+slider['height']/2
            page.mouse.move(x+slider['width']*.5,y);page.mouse.down()
            page.wait_for_function('cuiGallery.video.snapshot().requests.some(r=>r.kind===1&&r.phase===0)')
            page.mouse.move(x+slider['width']*.7,y);page.keyboard.press('Escape');page.mouse.up()
            page.wait_for_function('(origin)=>!cuiGallery.video.snapshot().seeking&&Math.abs(cuiGallery.video.snapshot().time-origin)<.1',arg=origin)
            assert any(r['kind']==1 and r['phase']==3 for r in snap()['requests'])
            # Physical surface pixels, not just timeline state, changed on actual decode.
            assert page.evaluate('cuiGallery.exports.gallery_count()>0')
            # Mobile/DPR2 layout still exposes the CUI controls within its logical root.
            page.set_viewport_size({'width':390,'height':844})
            page.wait_for_timeout(150)
            assert page.locator('#preview').evaluate('(c)=>c.width===Math.round(c.clientWidth*2)')
            assert page.evaluate('''()=>{let w=cuiGallery.exports;return Array.from({length:w.gallery_semantic_count()},(_,i)=>i).filter(i=>w.gallery_semantic(i,0)===1).every(i=>w.gallery_semantic(i,1)>=0&&w.gallery_semantic(i,1)+w.gallery_semantic(i,3)<=document.querySelector('#preview').clientWidth+1);}''')
            # Genuine stale-source/generation rejection at the staging boundary.
            state=snap()
            assert page.evaluate('s=>cuiGallery.exports.gallery_video_buffer(s.session+1,s.generation)',state)==0
            assert page.evaluate('s=>cuiGallery.exports.gallery_video_commit(s.session,s.generation+1,0)',state)==0
            # Deterministic rejected promise / late result are protocol tests,
            # separate from the real decoding and actual controls above.
            page.evaluate("""()=>{window.savedPlay=HTMLMediaElement.prototype.play;
                HTMLMediaElement.prototype.play=function(){return Promise.reject(Error('intentional test rejection'));};}""")
            action('Play')
            page.wait_for_function('cuiGallery.video.snapshot().error.includes("intentional test rejection")')
            assert snap()['paused']
            page.evaluate('()=>{HTMLMediaElement.prototype.play=window.savedPlay;}')
            # A late rejected old play operation cannot overwrite a newer success.
            page.evaluate("()=>{HTMLMediaElement.prototype.play=function(){return new Promise((resolve,reject)=>window.rejectOldPlay=reject);};}")
            action('Play');page.wait_for_function('typeof window.rejectOldPlay==="function"')
            page.evaluate('()=>{HTMLMediaElement.prototype.play=window.savedPlay;}')
            action('Play');page.wait_for_function('!cuiGallery.video.snapshot().paused')
            page.evaluate("window.rejectOldPlay(Error('stale rejection'))")
            page.wait_for_timeout(80);assert snap()['error']==''
            action('Pause');page.wait_for_function('cuiGallery.video.snapshot().paused')
            # Fullscreen can succeed or truthfully reject; it is a host capability.
            action('Enter fullscreen')
            page.wait_for_function('document.fullscreenElement || cuiGallery.video.snapshot().error.includes("Fullscreen")')
            if page.evaluate('!!document.fullscreenElement'):
                action('Leave fullscreen');page.wait_for_function('!document.fullscreenElement')
            old_url=snap()['source'];session=snap()['session'];page.evaluate('location.hash="Button"')
            page.wait_for_function('cuiGallery.renderedComponent==="Button" && !cuiGallery.video.snapshot().active')
            decoded=snap()['decodedFrames'];page.wait_for_timeout(400)
            assert snap()['decodedFrames']==decoded, 'no detached decoded-frame work'
            assert not page.evaluate('async url=>{try{await fetch(url);return true;}catch{return false;}}',old_url), 'route cleanup revokes Blob URL'
            page.evaluate('location.hash="Video"')
            page.wait_for_function('(session)=>cuiGallery.video.snapshot().session>session && cuiGallery.video.snapshot().frames>0',arg=session)
            assert snap()['paused']
            # Real queued keyboard requests overflow the bounded host batch;
            # no prefix is applied and the controls reset to confirmed state.
            before=snap();page.evaluate("""()=>{const w=cuiGallery.exports;for(let i=0;i<w.gallery_semantic_count();i++)if(w.gallery_semantic(i,0)===5)w.gallery_focus(i);
                const canvas=document.querySelector('#preview');for(let i=0;i<17;i++)canvas.dispatchEvent(new KeyboardEvent('keydown',{key:'ArrowRight',bubbles:true}));}""")
            page.wait_for_function('cuiGallery.video.snapshot().error.includes("entire batch rejected")')
            assert snap()['time']==before['time'] and snap()['paused']
            assert snap()['requests']==before['requests']
            # Delayed fetch completion after route exit cannot revive a decoder.
            pending=[]
            late=browser.new_page()
            late.route('**/assets/video/sample.webm',lambda route:pending.append(route))
            late.goto(f'http://127.0.0.1:{server.server_port}/cui/#Video')
            late.wait_for_function('window.cuiGallery?.video.snapshot().active')
            late.wait_for_timeout(50)
            late.evaluate('location.hash="Button"')
            late.wait_for_function('cuiGallery.renderedComponent==="Button"')
            for route in pending:
                try: route.fulfill(body=(root/'web/assets/video/sample.webm').read_bytes(),content_type='video/webm')
                except Exception: pass  # An aborted network request may already be disposed.
            late.wait_for_timeout(100)
            assert late.evaluate('!cuiGallery.video.snapshot().active && cuiGallery.video.snapshot().decodedFrames===0')
            late.close()
            # Actual alternate encoded asset decoding, after deliberately corrupt
            # preferred media, is distinct from synthetic host-error checks.
            alternate=browser.new_page()
            alternate.route('**/assets/video/sample.webm',lambda route:route.fulfill(body=b'not a WebM',content_type='video/webm'))
            alternate.goto(f'http://127.0.0.1:{server.server_port}/cui/#Video')
            alternate.wait_for_function('window.cuiGallery?.video.snapshot().frames>0')
            assert alternate.evaluate('cuiGallery.video.snapshot().asset.endsWith("sample.mp4")')
            alternate.close()
            oversized=browser.new_page()
            oversized.route('**/assets/video/sample.webm',lambda route:route.fulfill(body=b'x'*(512*1024+1),content_type='video/webm'))
            oversized.goto(f'http://127.0.0.1:{server.server_port}/cui/#Video')
            oversized.wait_for_function('window.cuiGallery?.video.snapshot().error.includes("512 KiB")')
            assert oversized.evaluate('cuiGallery.video.snapshot().frames===0')
            oversized.close()
            assert not errors,errors
            browser.close()
            print('PASS: actual decode/pixel change, CUI playback/seek/rate/keyboard, mobile DPR2, 78+extension, cleanup')
    finally:
        server.shutdown();server.server_close()

if __name__=='__main__':
    main()
