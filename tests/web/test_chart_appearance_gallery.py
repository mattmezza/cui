# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual gallery automatic/copied content across tooltip appearance choices."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

COMMANDS = """()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),
 rect:[1,2,3,4].map(j=>e.gallery_command(i,j)),alpha:e.gallery_command(i,8),
 text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
 e.gallery_command_length(i)))}));}"""

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
            audit=Audit(browser,f'http://127.0.0.1:{server.server_port}')
            for width in [1280,390]:
                audit.page.set_viewport_size({'width':width,'height':900})
                audit.route('Chart')
                assert audit.page.locator('#chart-tooltip-appearance').is_visible()
                assert audit.page.locator('#chart-tooltip-appearance').input_value()==('0' if width==1280 else '2')
                for custom in [False,True]:
                    audit.configure('chart-custom-rows',custom)
                    sizes=[]
                    for appearance in [0,1,2]:
                        audit.page.locator('#chart-tooltip-appearance').select_option(str(appearance))
                        audit.page.wait_for_function('cuiGallery.exports.gallery_chart_stats(7)==='+str(appearance))
                        target=audit.target('Project activity chart')
                        point=audit.point(target,.08,.4)
                        audit.page.mouse.move(point['x'],point['y'])
                        wanted='Reporting period' if custom else 'Project activity chart'
                        # Automatic fallback uses the chart's supplied series name.
                        audit.page.wait_for_function('('+COMMANDS+')().some(c=>c.type===2&&c.alpha>0&&c.text==='+repr(wanted)+')')
                        commands=audit.page.evaluate(COMMANDS)
                        row=next(c for c in commands if c['type']==2 and c['alpha']>0 and c['text']==wanted)
                        sizes.append(row['rect'][3])
                        if appearance==2:
                            # Bare content begins at the plot origin, with a swatch slot
                            # only for automatic series rows; plain copied rows have none.
                            assert row['rect'][0] < target['f'][1]+40,(target,row)
                        else:
                            assert row['rect'][0] > target['f'][1]+40,(target,row)
                        assert audit.page.evaluate('cuiGallery.exports.gallery_error()')==0
                    assert sizes[1]==sizes[2] and sizes[0]!=sizes[1],sizes
            audit.page.locator('#component-guides').get_by_role('link',name='Opt-in Chart tooltip appearance').click()
            audit.page.locator('#docs-title').filter(has_text='Opt-in Chart tooltip appearance').wait_for(state='visible')
            assert audit.page.get_by_role('heading',name='Opt-in Chart tooltip appearance',exact=True).count()==1
            assert audit.page.locator('#doc-opt-in-chart-tooltip-appearance').count()==1
            assert not audit.errors,audit.errors
            browser.close()
        print('PASS gallery classic/popover/bare typography and placement, automatic/copied rows, desktop/390px and linked guide')
    finally:
        server.shutdown()

if __name__=='__main__': main()
