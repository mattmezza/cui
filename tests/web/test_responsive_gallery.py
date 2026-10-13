#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM gallery content containment at mobile and desktop widths."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                 partial(Handler, directory=str(args.directory.resolve())))
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(
                headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 950})
            page.goto(f'http://127.0.0.1:{server.server_port}/#Button')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Button"')
            for width in (390, 1280):
                page.set_viewport_size({'width': width, 'height': 950})
                for family, sizes in (('Button', range(1, 15)),
                                      ('DescriptionList', range(1, 15)),
                                      ('Settings', range(1, 15))):
                    page.evaluate('(name)=>location.hash=name', family)
                    page.wait_for_function('(name)=>cuiGallery.renderedComponent===name', arg=family)
                    for size in sizes:
                        for density in (0, 1):
                            page.select_option('#design-size', str(size))
                            page.select_option('#density', str(density))
                            page.wait_for_timeout(60)
                            data = page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();
                                const canvas=document.getElementById('preview');
                                const texts=[];let clips=[];
                                const intersect=(a,b)=>({x:Math.max(a.x,b.x),
                                    y:Math.max(a.y,b.y),right:Math.min(a.right,b.right),
                                    bottom:Math.min(a.bottom,b.bottom)});
                                for(let i=0;i<e.gallery_count();i++){
                                    const f=j=>e.gallery_command(i,j),t=f(0);
                                    const b={x:f(1),y:f(2),right:f(1)+f(3),bottom:f(2)+f(4)};
                                    if(t===3){clips.push(clips.length?intersect(clips.at(-1),b):b);continue}
                                    if(t===4){clips.pop();continue}
                                    if(t===2)texts.push({name:d.decode(new Uint8Array(e.memory.buffer,
                                        e.gallery_command_text(i),e.gallery_command_length(i))),
                                        ...b,clip:clips.at(-1)||{x:0,y:0,right:canvas.clientWidth,
                                        bottom:canvas.clientHeight}});
                                }
                                return {texts,w:canvas.clientWidth,h:canvas.clientHeight};}''')
                            if family == 'Button':
                                loading = [t for t in data['texts'] if t['name'] == 'Loading']
                                assert loading, (width, size, density, data)
                                assert all(t['bottom'] <= data['h'] + 1 for t in loading), \
                                    (width, size, density, loading, data['h'])
                            else:
                                marker = 'Created' if family == 'DescriptionList' else 'Preferences'
                                selected = [t for t in data['texts'] if marker in t['name']]
                                assert selected, (family, width, size, density)
                                assert all(t['bottom'] <= data['h'] + 1 for t in selected), \
                                    (family, width, size, density, selected, data['h'])
                                if size <= 6:
                                    assert all(t['right'] <= t['clip']['right'] + 1
                                               for t in selected), \
                                        (family, width, size, density, selected)
                if width == 390 and family in ('DescriptionList', 'Settings'):
                    for size in ((12, 14) if family == 'DescriptionList' else (9, 12, 14)):
                        page.select_option('#design-size', str(size))
                        page.wait_for_timeout(80)
                        def region():
                            return page.evaluate('''()=>{const e=cuiGallery.exports;
                                for(let i=0;i<e.gallery_semantic_count();i++)
                                    if(e.gallery_semantic(i,20)>0)return {
                                        x:e.gallery_semantic(i,1),y:e.gallery_semantic(i,2),
                                        w:e.gallery_semantic(i,3),h:e.gallery_semantic(i,4),
                                        scroll:e.gallery_semantic(i,18),
                                        maximum:e.gallery_semantic(i,20)};
                                return null;}''')
                        area = region()
                        assert area and area['maximum'] > 30, (family, size, area)
                        label = 'Created' if family == 'DescriptionList' else 'Preferences'
                        right = page.evaluate('''label=>{const e=cuiGallery.exports,d=new TextDecoder();
                            let right=0;for(let i=0;i<e.gallery_count();i++){
                                if(e.gallery_command(i,0)!==2)continue;
                                const name=d.decode(new Uint8Array(e.memory.buffer,
                                    e.gallery_command_text(i),e.gallery_command_length(i)));
                                if(name===label)right=Math.max(right,e.gallery_command(i,1)+
                                    e.gallery_command(i,3));
                            }return right;}''', label)
                        assert right and right <= area['x'] + area['w'] + area['maximum'] + 1, \
                            (family, size, label, right, area)
                        canvas = page.locator('#preview').bounding_box()
                        page.mouse.move(canvas['x'] + area['x'] + min(area['w'], 200) / 2,
                                        canvas['y'] + area['y'] + min(area['h'], 100) / 2)
                        before = area['scroll']
                        page.mouse.wheel(90, 0)
                        page.wait_for_timeout(80)
                        assert region()['scroll'] > before, (family, size, before, region())
            browser.close()
    finally:
        server.shutdown()
    print('PASS: responsive content containment')


if __name__ == '__main__':
    main()
