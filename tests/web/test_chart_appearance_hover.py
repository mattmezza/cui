#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Chart mouse tooltip gating during appearance and reduced motion."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

POPUP = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),font:e.gallery_command(i,11),
 alpha:e.gallery_command(i,8),text:d.decode(new Uint8Array(e.memory.buffer,
 e.gallery_command_text(i),e.gallery_command_length(i)))})).some(c=>
 c.type===2&&c.font===11&&c.alpha>0&&c.text==='Jan');}'''

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory', type=Path, default=Path('build/web'))
    p.add_argument('--chromium')
    p.add_argument('--controlled-clock', action='store_true',
                   help='Advance the real browser host clock in 10ms render steps')
    args = p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            if args.controlled_clock:
                original_new_page = browser.new_page
                def controlled_page(**kwargs):
                    page = original_new_page(**kwargs)
                    page.add_init_script('''(()=>{let clock=0;
                        const raf=window.requestAnimationFrame.bind(window);
                        Object.defineProperty(performance,'now',{value:()=>clock});
                        window.requestAnimationFrame=fn=>raf(()=>{clock+=10;fn(clock)});})();''')
                    return page
                browser.new_page = controlled_page
            a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
            if args.controlled_clock:
                # One source second needs 100 genuine rendered browser frames.
                a.page.set_default_timeout(15000)
            a.page.emulate_media(reduced_motion='no-preference')
            a.route('Chart')
            chart = a.target('Project activity chart')
            point = a.point(chart, .08, .4)
            a.configure('chart-animate', True)
            a.page.mouse.move(point['x'], point['y'])
            a.page.wait_for_timeout(150)
            progress = a.page.evaluate('cuiGallery.exports.gallery_chart_stats(2)')
            assert 0 < progress < 1, progress
            assert not a.page.evaluate(POPUP), 'Tooltip appeared before marks completed'
            # No second pointer event: pending valid cursor resumes after completion.
            a.page.wait_for_function('cuiGallery.exports.gallery_chart_stats(2)===1')
            a.page.wait_for_function(POPUP)
            a.page.mouse.move(point['x'] + chart['f'][3] + 80, point['y'])
            a.page.wait_for_timeout(180)
            assert not a.page.evaluate(POPUP), 'Tooltip exit failed to settle'
            # Fresh appearance plus reduced motion completes immediately.
            a.configure('chart-animate', False)
            a.page.emulate_media(reduced_motion='reduce')
            a.configure('chart-animate', True)
            a.page.mouse.move(point['x'], point['y'])
            a.page.wait_for_timeout(60)
            assert a.page.evaluate('cuiGallery.exports.gallery_chart_stats(2)') == 1
            assert a.page.evaluate(POPUP), 'Reduced motion incorrectly blocked hover'
            assert a.page.evaluate('cuiGallery.exports.gallery_error()') == 0
            assert not a.errors, a.errors
            browser.close()
    finally:
        server.shutdown()
    print('Chart appearance hover browser passed')

if __name__ == '__main__':
    main()
