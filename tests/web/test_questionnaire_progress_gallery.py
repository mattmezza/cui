#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual WASM progress presentation, numeric semantics and copyable C usage."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

ROOT = Path(__file__).resolve().parents[2]


def compile_usage(text):
    with tempfile.TemporaryDirectory(prefix='cui-progress-usage-') as directory:
        source = Path(directory) / 'usage.c'
        source.write_text(text)
        result = subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-fsyntax-only',
            '-I' + str(ROOT / 'include'), str(source)], text=True, capture_output=True)
        assert result.returncode == 0, result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True)
            for mobile in (False, True):
                a = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                if mobile:
                    a.page.set_viewport_size({'width': 390, 'height': 844})
                a.route('Questionnaire')
                assert a.page.locator('#questionnaire-progress').input_value() == '0'
                base = a.target('Question 1 of 2')
                role = base['f'][0]
                assert base['f'][8] == 1 and base['f'][12:14] == [0, 2]

                def node():
                    nodes = [n for n in a.snapshot() if n['f'][0] == role]
                    assert len(nodes) == 1, nodes
                    return nodes[0]

                def commands():
                    return a.page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>({text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),f:Array.from({length:12},(_,f)=>e.gallery_command(i,f))}));}''')

                for mode in range(1, 5):
                    a.page.locator('#questionnaire-progress').select_option(str(mode))
                    a.page.wait_for_function('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_semantic_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))).includes('Frage 1 von 2')}''')
                    a.page.wait_for_timeout(60)
                    n = node()
                    assert n['id'] == base['id'] and n['f'][8] == 1 and n['f'][12:14] == [0, 2]
                    assert n['f'][7] == 0
                    text = a.texts()
                    if mode in (2, 3):
                        assert '1 / 2' in text and 'Frage 1 von 2' not in text
                    else:
                        assert 'Frage 1 von 2' in text
                    if mode == 3:
                        assert any(c['f'][0] == 0 and abs(c['f'][3] - 96) < .1 and
                                   abs(c['f'][4] - 4) < .1 for c in commands())
                    if mode == 4:
                        c = next(c for c in commands() if c['text'] == 'Frage 1 von 2')
                        assert c['f'][11] == 18 and c['f'][4] == 28
                    canvas_width = a.page.evaluate('cuiGallery.exports.gallery_command ? document.querySelector("canvas").getBoundingClientRect().width : 0')
                    assert n['f'][1] >= -1 and n['f'][1] + n['f'][3] <= canvas_width + 1
                    compile_usage(a.page.locator('#snippet').inner_text())
                    assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(4)') == 0
                # Selecting a presentation alone emits no answer change. Actual
                # desktop pointer or CDP touch selects/advances the retained model.
                target = a.target('Explore components', 4)
                if mobile:
                    a.tap(target)
                else:
                    point = a.point(target)
                    a.page.mouse.click(point['x'], point['y'])
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===1')
                a.page.wait_for_function('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_semantic_count()},(_,i)=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))).includes('Frage 2 von 2')}''')
                assert node()['f'][8] == 2 and node()['f'][12:14] == [0, 2]
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(4)') == 1
                a.page.locator('#questionnaire-progress').select_option('0')
                a.page.wait_for_timeout(80)
                assert node()['name'] == 'Question 2 of 2'
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS real progress modes/semantics/snippets/navigation; touch={mobile}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
