# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM current-frame Button label ellipsis and canvas containment."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
from test_button_activation_blocker import HTML
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'build/button-label-probe')
    parser.add_argument('--chromium', default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args(); out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    tree = ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'SOURCES' for t in n.targets))
    sources = [str(ROOT/s) for s in sources if s.startswith('src/')]
    sources += [str(ROOT/'src/base/cui_button_group.c'), str(ROOT/'backends/software/cui_software.c')]
    sources = list(dict.fromkeys(sources))
    exports = ['probe_init','probe_event','probe_key','probe_frame','probe_page_available','probe_page_end','probe_stat','probe_rect','probe_count','probe_command','probe_text','probe_text_length','probe_bitmap','label_width','label_semantic']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O1','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),str(ROOT/'tests/web/button_label_probe.c'),*sources,'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={e}' for e in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args): pass
    server = ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(out)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page(viewport={'width':480,'height':600},has_touch=True)
            errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe?.frames>=3')
            for mode in [0,1]:
                for width in [80,160,70,340]:
                    page.evaluate('([m,w])=>{probe.e.label_width(w);probe.reset(m)}',[mode,width])
                    snapshot=page.evaluate("""()=>{let e=probe.e;return {name:e.label_semantic(),bounds:Array.from({length:4},(_,i)=>e.probe_rect(20,i)),texts:Array.from({length:e.probe_count()},(_,i)=>({type:e.probe_command(i,0),bounds:Array.from({length:4},(_,j)=>e.probe_command(i,j+1)),text:new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_text_length(i)))})).filter(c=>c.type===2)}}""")
                    assert snapshot['name']==1, snapshot
                    b=snapshot['bounds']; assert abs(b[2]-width)<.1, snapshot
                    text=''.join(c['text'] for c in snapshot['texts'])
                    assert text.endswith('…') and '\n' not in text, snapshot
                    for c in snapshot['texts']:
                        x,y,w,h=c['bounds'];assert x>=b[0]-.01 and y>=b[1]-.01 and x+w<=b[0]+b[2]+.01 and y+h<=b[1]+b[3]+.01,snapshot
                    pixels=page.evaluate("""()=>{const c=document.querySelector('canvas');const a=c.getContext('2d').getImageData(0,0,360,80).data;let outside=0,ink=0;let width=probe.e.probe_rect(20,2);for(let y=0;y<80;y++)for(let x=0;x<360;x++){let i=(y*360+x)*4;if(a[i+3]){if(x>=Math.ceil(width))outside++;else ink++;}}return {outside,ink}}""")
                    assert pixels['outside']==0 and pixels['ink']>100,pixels
                    tail=next(c for c in snapshot['texts'] if c['text']=='…')
                    dots=page.evaluate("""b=>{const ctx=document.querySelector('canvas').getContext('2d');const x=Math.floor(b[0]),y=Math.floor(b[1]),w=Math.ceil(b[2])+1,h=Math.ceil(b[3])+1;const a=ctx.getImageData(x,y,w,h).data;let n=0;for(let i=0;i<a.length;i+=4)if(a[i]>220&&a[i+1]>220&&a[i+2]>220&&a[i+3])n++;return n}""",tail['bounds'])
                    assert dots>0,('ellipsis has no visible glyph pixels',snapshot)
                page.mouse.click(30,25);page.wait_for_function('probe.data[1]===1')
                cdp=page.context.new_cdp_session(page)
                cdp.send('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[{'x':30,'y':25,'id':1}]})
                cdp.send('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
                page.wait_for_function('probe.data[1]===2')
                assert page.evaluate('probe.data[1]')==2
            assert not errors,errors
            page.screenshot(path=str(out/'button-label.png'))
            browser.close()
        print('PASS: fixed/grow, four current-frame widths, full semantics, ellipsis, actual pixels, mouse/touch')
    finally: server.shutdown()
if __name__=='__main__': main()
