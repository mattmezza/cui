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
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Video" && cuiGallery.video.snapshot().frames>0 && !cuiGallery.video.snapshot().paused')
            assert page.evaluate('cuiGallery.catalog.components.length')==78
            assert page.evaluate('cuiGallery.catalog.components.every(c=>!["Video","VideoPlayer"].includes(c.name))')
            assert page.evaluate('cuiGallery.catalog.extensions.map(c=>c.name)')==['Video','VideoPlayer']
            assert all(len(c['source_commit'])==40 for c in page.evaluate('cuiGallery.catalog.extensions'))
            page.locator('#search').fill('video')
            for name in ('Video','VideoPlayer'):
                assert page.locator(f'#catalog a[href="#{name}"]').count()==1
            page.locator('#search').fill('')
            assert page.locator('.version').inner_text()==version
            assert page.locator('video').count()==0
            snippet=page.locator('#snippet').inner_text()
            assert 'cui_video' in snippet and 'cui_video_player' not in snippet
            def snap(): return page.evaluate('cuiGallery.video.snapshot()')
            def surface_hash(): return page.evaluate("""()=>{const c=document.querySelector('#preview'),d=c.width/c.clientWidth;
                const bytes=c.getContext('2d').getImageData(24*d,24*d,160*d,90*d).data;
                let hash=2166136261;for(let i=0;i<bytes.length;i+=97)hash=Math.imul(hash^bytes[i],16777619);return hash>>>0;}""")
            first=snap();pixels=surface_hash()
            assert first['muted'] and first['loop'] and first['duration']>=11
            assert first['source'].startswith('blob:')
            page.wait_for_function('(time)=>cuiGallery.video.snapshot().time>time+.3',arg=first['time'])
            page.wait_for_function('(hash)=>cuiGallery.video.snapshot().checksum!==hash',arg=first['checksum'])
            assert surface_hash()!=pixels, 'actual bare Video canvas pixels must advance'
            assert page.locator('#accessible [role="slider"]').count()==0
            for label in ('Play','Pause','Skip backward','Skip forward','Enter fullscreen','Playback rate'):
                assert page.locator('#accessible').get_by_role('button',name=label,exact=True).count()==0
            # Bare Video remains a bounded scene even on a mobile viewport.
            for width in (320,390):
                page.set_viewport_size({'width':width,'height':844})
                page.wait_for_timeout(150)
                assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
                assert page.locator('#preview').evaluate('(c)=>c.getBoundingClientRect().height<innerHeight')
            old_url=snap()['source'];session=snap()['session']
            page.evaluate('location.hash="Button"')
            page.wait_for_function('cuiGallery.renderedComponent==="Button" && !cuiGallery.video.snapshot().active')
            frames=snap()['decodedFrames'];page.wait_for_timeout(250)
            assert snap()['decodedFrames']==frames
            assert not page.evaluate('async url=>{try{await fetch(url);return true;}catch{return false;}}',old_url)
            page.evaluate('location.hash="Video"')
            page.wait_for_function('(session)=>cuiGallery.video.snapshot().session>session && cuiGallery.video.snapshot().frames>0 && !cuiGallery.video.snapshot().paused',arg=session)
            assert not errors,errors
            browser.close()
            print('PASS: bare Video real decode/autoplay/pixels, no default controls, 78+2 extensions and source cleanup')
    finally:
        server.shutdown();server.server_close()

if __name__=='__main__':
    main()
