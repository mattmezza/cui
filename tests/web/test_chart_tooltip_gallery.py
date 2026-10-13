#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real gallery callback rows, desktop/mobile pixels, and runtime opt-out."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import re
import subprocess
import tempfile
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

ROW = '''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),
 rect:[1,2,3,4].map(j=>e.gallery_command(i,j)),alpha:e.gallery_command(i,8),
 text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
 e.gallery_command_length(i)))})).find(c=>c.type===2&&c.alpha>0&&c.text==='Reporting period');}'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    example=re.search(r'```c\n(.*?)\n```',(root/'docs/CHART_TYPED_TOOLTIP.md').read_text(),re.S).group(1)
    with tempfile.TemporaryDirectory(prefix='cui-tooltip-example-') as temporary:
        source=Path(temporary)/'example.c'
        source.write_text(example+'\nint main(void) { cui_chart_options o = {0}; o.tooltip_build = tooltip; return o.tooltip_build == NULL; }\n')
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(root/'include'),'-c',str(source),'-o',str(Path(temporary)/'example.o')],check=True)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):
            pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            audit=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            audit.page.set_default_timeout(10000)
            for width in [1280,390]:
                audit.page.set_viewport_size({'width':width,'height':900})
                audit.route('Chart')
                assert audit.page.locator('#chart-custom-rows-control').is_visible()
                audit.configure('chart-custom-rows',True)
                audit.page.wait_for_function('cuiGallery.exports.gallery_chart_stats(6)===1')
                target=audit.target('Project activity chart')
                point=audit.point(target,.08,.4)
                audit.page.mouse.move(point['x'],point['y'])
                audit.page.wait_for_function(ROW)
                assert '2026' in audit.texts()
                assert audit.page.evaluate('cuiGallery.exports.gallery_chart_stats(5)')>0
                row=audit.page.evaluate(ROW)
                assert audit.page.evaluate('''rect=>{const c=document.querySelector('canvas'),
                    scale=c.width/c.clientWidth,r=rect.map(v=>v*scale),
                    data=c.getContext('2d').getImageData(Math.ceil(r[0]),Math.ceil(r[1]),
                    Math.max(1,Math.floor(r[2])),Math.max(1,Math.floor(r[3]))).data;
                    const colors=new Set();for(let i=0;i<data.length;i+=4)
                    colors.add(`${data[i]},${data[i+1]},${data[i+2]}`);return colors.size>1;}''',row['rect']),row
                audit.configure('chart-custom-rows',False)
                audit.page.wait_for_function('cuiGallery.exports.gallery_chart_stats(6)===0')
                audit.page.wait_for_function('!('+ROW+')()')
                assert audit.page.evaluate('cuiGallery.exports.gallery_error()')==0
            audit.page.context.grant_permissions(['clipboard-read','clipboard-write'])
            audit.page.locator('#component-guides').get_by_role('link',name='Copied Chart tooltip content').click()
            button=audit.page.get_by_role('button',name='Copy code block').first
            button.click()
            audit.page.wait_for_function('document.querySelector(".docs-copy").textContent==="Copied"')
            copied=audit.page.evaluate('navigator.clipboard.readText()')
            assert '#include "cui/cui_chart.h"' in copied and 'cui_chart_tooltip_add_row' in copied,copied
            assert not audit.errors,audit.errors
            browser.close()
        print('PASS custom gallery tooltip rows: desktop/390px callback, canvas ink, opt-out and copied compilable guide')
    finally:
        server.shutdown()
if __name__=='__main__':
    main()
