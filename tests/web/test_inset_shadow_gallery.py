# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Require actual Theme gallery INSET serialization, painted pixels and DPR."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            for scale in [1, 2]:
                context = browser.new_context(viewport={'width': 1280, 'height': 1000}, device_scale_factor=scale)
                page = context.new_page(); errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Theme')
                try:
                    page.wait_for_function('window.cuiGallery&&cuiGallery.exports.gallery_count()>0')
                except Exception:
                    print('Gallery startup', errors, page.locator('#preview-note').inner_text(), flush=True)
                    raise
                page.wait_for_function('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>e.gallery_command(i,0)).includes(8)}')
                result = page.evaluate('''scale=>{
                    const e=cuiGallery.exports,ctx=document.querySelector('canvas').getContext('2d');
                    const index=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===8);
                    const values=Array.from({length:16},(_,f)=>e.gallery_shadow_value(index,f));
                    const [x,y,w,h]=values;
                    const pixel=(px,py)=>Array.from(ctx.getImageData(Math.floor(px*scale),Math.floor(py*scale),1,1).data);
                    return {values,edge:pixel(x+2,y+h/2),center:pixel(x+w/2,y+h/2),error:e.gallery_error(),
                            dropBefore:Array.from({length:index},(_,i)=>e.gallery_command(i,0)).includes(7)};
                }''', scale)
                assert result['error'] == 0 and not errors, (result, errors)
                assert result['dropBefore'] and result['values'][15] == 96, result
                assert all(isinstance(value, (int, float)) for value in result['values']), result
                assert result['center'] == [29, 100, 220, 255], result
                assert result['edge'][2] < 210 and result['edge'][3] == 255, result
                context.close()
            browser.close()
    finally: server.shutdown()
    print('Theme gallery mixed DROP/INSET values and actual edge/center pixels DPR1/2 PASS')
if __name__ == '__main__': main()
