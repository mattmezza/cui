#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real clipboard code follows selected joined Button and password modes."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import tempfile
import threading
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0),
        partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw, tempfile.TemporaryDirectory() as temporary:
            browser = pw.chromium.launch(headless=True,
                **({'executable_path': args.chromium} if args.chromium else {}))
            for width in [1280, 390]:
                page = browser.new_page(viewport={'width': width, 'height': 900},
                    permissions=['clipboard-read', 'clipboard-write'])
                errors = []
                page.on('pageerror', lambda error: errors.append(str(error)))
                for name, modes in [('Button', [1, 2, 3]), ('Input', [4, 5, 6, 7, 8])]:
                    page.goto(f'http://127.0.0.1:{server.server_port}/#{name}')
                    page.wait_for_function('(name)=>window.cuiGallery&&cuiGallery.renderedComponent===name', arg=name)
                    control = '#button-collection' if name == 'Button' else '#input-mode'
                    for mode in modes:
                        page.locator(control).select_option(str(mode))
                        page.locator('#design-size').select_option('5')
                        page.locator('#disabled').check()
                        expected = ['.design_size = CUI_DESIGN_LG', '.disabled = true']
                        if name == 'Button':
                            expected += [f'.multiple = {str(mode == 2).lower()}',
                                         f'.vertical = {str(mode == 3).lower()}']
                        elif mode >= 7:
                            expected += ['CUI_EDIT_FORMAT_PATTERN' if mode == 7 else 'CUI_EDIT_FORMAT_NUMBER']
                        else:
                            expected += [f'.masked = {str(mode != 5).lower()}',
                                         'NEW_PASSWORD' if mode == 6 else 'PURPOSE_PASSWORD']
                        page.wait_for_function('(expected)=>expected.every(part=>document.querySelector("#snippet").textContent.includes(part))', arg=expected)
                        code = page.locator('#snippet').text_content()
                        if name == 'Button':
                            assert f'.multiple = {str(mode == 2).lower()}' in code
                            assert f'.vertical = {str(mode == 3).lower()}' in code
                            assert '.change_user = view' in code
                        elif mode >= 7:
                            assert ('CUI_EDIT_FORMAT_PATTERN' if mode == 7 else 'CUI_EDIT_FORMAT_NUMBER') in code
                            assert '.format_workspace = {workspace, capacity}' in code
                        else:
                            assert f'.masked = {str(mode != 5).lower()}' in code
                            assert ('NEW_PASSWORD' if mode == 6 else 'PURPOSE_PASSWORD') in code
                        page.locator('#copy-snippet').click()
                        page.wait_for_function('()=>document.querySelector("#copy-snippet").textContent==="Copied"')
                        assert page.evaluate('navigator.clipboard.readText()') == code
                        source = Path(temporary) / 'copied.c'
                        source.write_text(code)
                        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-pedantic', '-fsyntax-only', '-I', str(ROOT / 'include'),
                            str(source)], check=True, capture_output=True)
                        page.locator('#disabled').uncheck()
                    page.locator(control).select_option('0')
                    page.wait_for_function('()=>!document.querySelector("#snippet").textContent.includes("#include")')
                assert not errors, errors
                page.close()
            browser.close()
    finally:
        server.shutdown()
    print('PASS: 16 desktop/mobile actual clipboard examples match selected modes, size and disabled state; all copied C11 units compile')


if __name__ == '__main__':
    main()
