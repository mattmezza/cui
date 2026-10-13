#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real gallery folder-alpha clipping through expanded/collapsed sidebar scroll."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(**({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 900})
            page.goto(f'http://127.0.0.1:{server.server_port}/#Sidebar')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Sidebar"')
            snapshot = '''()=>{const e=cuiGallery.exports,decoder=new TextDecoder();
                const semantic=label=>{for(let i=0;i<e.gallery_semantic_count();i++){
                    const name=decoder.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)));
                    if(name===label)return [1,2,3,4].map(f=>e.gallery_semantic(i,f));}throw Error(label);};
                const viewport=semantic('Scrollable content'),files=semantic('Files');
                const commands=Array.from({length:e.gallery_count()},(_,i)=>({i,t:e.gallery_command(i,0),b:[1,2,3,4].map(f=>e.gallery_command(i,f))}));
                return {viewport,files,commands};}'''
            for collapsed in (False, True):
                if collapsed:
                    page.locator('#sidebar-collapsed').check()
                    page.wait_for_function('Math.abs(cuiGallery.exports.gallery_sidebar_stats(1)-48)<.2')
                data = page.evaluate(snapshot)
                files = data['files']
                icons = [c for c in data['commands'] if c['t'] == 5 and
                         files[0] <= c['b'][0] + c['b'][2]/2 < files[0] + files[2] and
                         files[1] <= c['b'][1] + c['b'][3]/2 < files[1] + files[3]]
                assert icons, data
                folder = page.evaluate('(i)=>cuiGallery.exports.gallery_bitmap_pixels(i)', icons[0]['i'])
                for delta in (0, 17.5, 90, 160):
                    if delta:
                        box = page.locator('canvas').bounding_box()
                        v = data['viewport']
                        page.mouse.move(box['x'] + v[0] + v[2]/2, box['y'] + v[1] + v[3]/2)
                        before = page.evaluate('cuiGallery.exports.gallery_sidebar_stats(2)')
                        page.mouse.wheel(0, delta)
                        page.wait_for_function('(before)=>cuiGallery.exports.gallery_sidebar_stats(2)>before', arg=before)
                    result = page.evaluate('''({folder,viewport})=>{const e=cuiGallery.exports;
                        const intersect=(a,b)=>{const x=Math.max(a[0],b[0]),y=Math.max(a[1],b[1]);return [x,y,Math.max(0,Math.min(a[0]+a[2],b[0]+b[2])-x),Math.max(0,Math.min(a[1]+a[3],b[1]+b[3])-y)];};
                        const inside=(x,y,b)=>x>=b[0]&&x<b[0]+b[2]&&y>=b[1]&&y<b[1]+b[3];
                        const stack=[[0,0,10000,10000]];let tested=0,visible=0,leaked=0;
                        for(let i=0;i<e.gallery_count();i++){
                            const t=e.gallery_command(i,0),b=[1,2,3,4].map(f=>e.gallery_command(i,f));
                            if(t===3)stack.push(intersect(stack.at(-1),b));
                            else if(t===4){if(stack.length===1)throw Error('unbalanced pop');stack.pop();}
                            else if(t===5&&e.gallery_bitmap_pixels(i)===folder){
                                tested++;const w=e.gallery_bitmap(i,0),h=e.gallery_bitmap(i,1),stride=e.gallery_bitmap(i,2);
                                const pixels=new Uint8Array(e.memory.buffer,folder,stride*h);
                                for(let y=0;y<h;y++)for(let x=0;x<w;x++)if(pixels[y*stride+x*4+3]){
                                    const px=b[0]+(x+.5)*b[2]/w,py=b[1]+(y+.5)*b[3]/h;
                                    if(inside(px,py,stack.at(-1))){visible++;if(!inside(px,py,viewport))leaked++;}
                                }
                            }
                        }if(stack.length!==1)throw Error('unbalanced stack');return {tested,visible,leaked};}''',
                                           {'folder': folder, 'viewport': data['viewport']})
                    assert result['leaked'] == 0, (collapsed, delta, result)
                    if delta == 0:
                        assert result['tested'] and result['visible'], result
                # Return to the initial source position before changing collapse mode.
                page.mouse.wheel(0, -10000)
                page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(2)===0')
            browser.close()
            print('PASS: actual folder alpha respects body viewport above header/below footer in both sidebar modes')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
