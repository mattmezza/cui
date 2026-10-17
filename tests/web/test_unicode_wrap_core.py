# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Core contextual UTF-8 wrapping with real Chromium Canvas measurements."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
HTML = '''<!doctype html><canvas></canvas><script>
(async()=>{
 const module=await WebAssembly.compile(await(await fetch('probe.wasm')).arrayBuffer());
 const ctx=document.querySelector('canvas').getContext('2d'), decoder=new TextDecoder('utf-8',{fatal:true});
 let e; let measures=0;
 function measure(text,size=16){ctx.font=`${size}px monospace`;ctx.fontKerning='none';return ctx.measureText(text).width;}
 const imports={};for(const i of WebAssembly.Module.imports(module)){
  (imports[i.module]??={})[i.name]=i.name==='canvas_width'?(ptr,length,size)=>{
   ++measures;return measure(decoder.decode(new Uint8Array(e.memory.buffer,ptr,length)),size);
  }:i.name==='proc_exit'?()=>{throw Error('WASI exit')}:()=>0;
 }
 e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();
 if(e.probe_init())throw Error('init');
 window.probe={e,measure,snapshot(sample,width){
  if(e.probe_frame(sample,width))throw Error('frame');
  const bytes=new Uint8Array(e.memory.buffer,e.probe_text(),e.probe_length());
  const text=decoder.decode(bytes),encode=new TextEncoder();
  const segments=Array.from(new Intl.Segmenter('en',{granularity:'grapheme'}).segment(text));
  const boundaries=new Set(segments.map(s=>encode.encode(text.slice(0,s.index)).length));boundaries.add(bytes.length);
  const rows=Array.from({length:e.probe_count()},(_,i)=>{
   const f=Array.from({length:10},(_,field)=>e.probe_row(i,field));
   const line=decoder.decode(new Uint8Array(e.memory.buffer,e.probe_row_text(i),f[8]));
   return {f,line,paragraph:decoder.decode(new Uint8Array(e.memory.buffer,e.probe_row_paragraph(i),f[7])),measured:measure(line),clusters:Array.from(new Intl.Segmenter('en',{granularity:'grapheme'}).segment(line)).length,
    valid:boundaries.has(f[0])&&boundaries.has(f[1])&&!!e.probe_boundary(f[0])&&!!e.probe_boundary(f[1]),
    spans:Array.from({length:f[6]},(_,j)=>Array.from({length:3},(_,field)=>e.probe_span(i,j,field)))};
  });
  return {text,length:bytes.length,width:e.probe_width(),split:e.probe_split(),rows,measures};
 }};
})().catch(error=>{window.probeError=String(error);throw error;});
</script>'''


def verify(snapshot):
    rows = snapshot['rows']
    assert rows and snapshot['measures'] > 0, snapshot
    assert ''.join(row['line'] for row in rows) == snapshot['text'], snapshot
    offset = 0
    for index, row in enumerate(rows):
        start, end, x, y, width, height, _, paragraph_length, length, kind = row['f']
        assert kind == 2 and row['valid'] and row['paragraph'] == snapshot['text'], row
        assert start == offset and end - start == length and paragraph_length == snapshot['length'], row
        assert x == 4 and y == 4 + index * 20 and height == 20, row
        if width > snapshot['width'] + .1:
            assert row['clusters'] == 1, (snapshot, row)
        assert row['measured'] <= width + .2, row
        expected = []
        for a, b, red in ((0, snapshot['split'], 200), (snapshot['split'], snapshot['length'], 10)):
            if max(a, start) < min(b, end):
                expected.append([max(a, start), min(b, end), red])
        assert row['spans'] == expected, row
        offset = end
    assert offset == snapshot['length'], snapshot


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/unicode-wrap-core-probe')
    parser.add_argument('--core-root', type=Path, default=ROOT, help='Optional older Core/Clay tree for regression proof')
    parser.add_argument('--chromium', default=str(Path.home() / '.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    core = args.core_root.resolve()
    assert subprocess.check_output(['zig', 'version'], text=True).strip() == '0.16.0'
    exports = ['probe_init', 'probe_frame', 'probe_count', 'probe_text', 'probe_length',
               'probe_width', 'probe_split', 'probe_boundary', 'probe_row', 'probe_row_text',
               'probe_row_paragraph', 'probe_span']
    subprocess.run(['zig', 'cc', '-target', 'wasm32-wasi', '-mexec-model=reactor', '-std=c11', '-O1',
                    '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(core / 'include'),
                    '-I', str(core / 'third_party/clay'), str(ROOT / 'tests/web/unicode_wrap_fixture.c'),
                    str(core / 'src/core/cui.c'), '-lm', '-Wl,-z,stack-size=8388608',
                    *[f'-Wl,--export={name}' for name in exports], '-o', str(out / 'probe.wasm')], check=True)
    (out / 'index.html').write_text(HTML)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(out)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True, executable_path=args.chromium)
            page = browser.new_page()
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}')
            page.wait_for_function('window.probe || window.probeError')
            assert not page.evaluate('window.probeError'), errors
            for sample in range(6):
                snapshots = []
                for width in (240, 72, 28, 9, 160, 72):
                    value = page.evaluate('([sample,width])=>probe.snapshot(sample,width)', [sample, width])
                    assert abs(value['width'] - (width - 8)) < .01, value
                    verify(value)
                    snapshots.append(value)
                assert len(snapshots[1]['rows']) > len(snapshots[0]['rows']), snapshots
                assert [r['line'] for r in snapshots[1]['rows']] == [r['line'] for r in snapshots[-1]['rows']], snapshots
            assert not errors, errors
            browser.close()
    finally:
        server.shutdown()
    print('Core Unicode wrapping with real Canvas measurements PASS (six cases, six current widths)')


if __name__ == '__main__':
    main()
