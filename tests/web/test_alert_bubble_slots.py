#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM Alert/Bubble semantic colors, alignment and settled reaction geometry."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
PAINT = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),b:[1,2,3,4].map(f=>e.gallery_command(i,f)),color:[5,6,7,8].map(f=>e.gallery_command(i,f)),text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))}));}'''
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,default=Path('build/web'))
    p.add_argument('--chromium')
    args=p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            for viewport in [1280,390]:
                a.page.set_viewport_size({'width':viewport,'height':1200})
                a.route('Alert')
                paints=a.page.evaluate(PAINT)
                messages=['A new version is available.','You can keep working.','Archive unused projects to free space.','Check your connection and try again.']
                text=[next(c for c in paints if c['text'].startswith(m[:10])) for m in messages]
                assert len({tuple(c['color']) for c in text})==4
                assert all(c['b'][2]>0 for c in text)
                a.route('Bubble');a.page.wait_for_timeout(200)
                paints=a.page.evaluate(PAINT)
                text=[c for c in paints if c['type']==2 and any(c['text'].startswith(k+':') for k in ['Info','Success','Warning','Error'])]
                assert len(text)==4 and len({tuple(c['color']) for c in text})==4,(viewport,text)
                assert text[1]['b'][0]>text[0]['b'][0]
                reactions=[c for c in paints if c['text']=='Liked · 2'];assert len(reactions)==4
                assert reactions[0]['b'][1]<text[0]['b'][1]
                assert reactions[1]['b'][1]>text[1]['b'][1]
                assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
                target=a.target('Liked · 2')
                a.tap(target)
                assert 'Liked · 3' in a.texts()
                a.tap(a.target('Liked · 3'))
                assert len([s for s in a.snapshot() if s['name']=='Liked · 2'])==4
            assert not a.errors,a.errors
            browser.close()
    finally: server.shutdown()
    print('Alert/Bubble browser slots passed')
if __name__=='__main__':main()
