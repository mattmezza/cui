#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0", "wasmtime==49.0.0"]
# ///
"""Actual gallery DOM/CDP -> CUI Input -> optional compiled Unicode provider."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_input_acceptance_gallery import TEXTS


def serve(directory):
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0),
        partial(Handler, directory=str(directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def headless(directory, available):
    import wasmtime
    config = wasmtime.Config()
    config.consume_fuel = True
    engine = wasmtime.Engine(config)
    with wasmtime.Store(engine) as store:
        store.set_fuel(80000000000)
        wasi = wasmtime.WasiConfig()
        wasi.inherit_stdout()
        wasi.inherit_stderr()
        store.set_wasi(wasi)
        with wasmtime.Linker(engine) as linker:
            linker.define_wasi()
            with wasmtime.Module.from_file(engine, str(directory / 'gallery.wasm')) as module:
                # Deterministic headless metrics match gallery's native fallback.
                # Browser proof below independently uses actual Canvas metrics.
                for imported in module.imports:
                    if imported.module == 'host':
                        assert imported.name in ('measure', 'measure_styled', 'font_metrics')
                        callback = (lambda *args: 0) if imported.name == 'font_metrics' else (lambda *args: args[1] * args[2] * .6)
                        linker.define(store, 'host', imported.name, wasmtime.Func(store, imported.type, callback))
                e = linker.instantiate(store, module).exports(store)
                def call(name, *args):
                    return e[name](store, *args)
                call('_initialize')
                assert call('gallery_init') == 1
                assert call('gallery_regex_available') == available
                call('gallery_input_mode', 9)
                def frame():
                    assert call('gallery_frame', 6, 0, 0, 720.0, 300.0) == 1
                frame()
                target = next(i for i in range(call('gallery_semantic_count'))
                              if call('gallery_semantic', i, 0) == 6)
                call('gallery_focus', target)
                frame()
                call('gallery_event', 3, 0.0, 0.0, 13, 2, 0, 0, 1)  # Ctrl+A
                frame()
                def text(value):
                    data = value.encode()
                    ptr = call('malloc', len(data))
                    assert ptr
                    e['memory'].write(store, data, ptr)
                    call('gallery_text', ptr, len(data))
                    frame()
                    call('free', ptr)
                text('Ada Δ')
                assert call('gallery_input_selection', 2) == 6
                text('!7')
                assert call('gallery_input_selection', 2) == (6 if available else 8)
                assert call('gallery_error') == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--default-directory', type=Path, required=True)
    parser.add_argument('--chromium')
    args = parser.parse_args()
    headless(args.directory, 1)
    headless(args.default_directory, 0)
    linked, default = serve(args.directory), serve(args.default_directory)
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True,
                **({'executable_path': args.chromium} if args.chromium else {}))
            plain = Audit(browser, f'http://127.0.0.1:{default.server_port}')
            plain.route('Input')
            assert plain.page.evaluate('cuiGallery.exports.gallery_regex_available()') == 0
            assert plain.page.locator('#input-mode option[value="9"]').is_disabled()
            plain.page.evaluate('cuiGallery.exports.gallery_input_mode(9);cuiGallery.render()')
            assert plain.target('Display name', 6)
            assert not plain.errors, plain.errors
            plain.page.close()
            for width in (1280, 390):
                audit = Audit(browser, f'http://127.0.0.1:{linked.server_port}')
                audit.page.set_viewport_size({'width': width, 'height': 900})
                audit.route('Input')
                assert audit.page.evaluate('cuiGallery.exports.gallery_regex_available()') == 1
                assert not audit.page.locator('#input-mode option[value="9"]').is_disabled()
                audit.configure('input-mode', 9)
                field = audit.target('Unicode name', 6)
                point = audit.point(field, .25, .5)
                audit.page.mouse.click(point['x'], point['y'])
                audit.page.wait_for_function('document.activeElement.id==="cui-ime-input"')
                audit.page.keyboard.press('Control+a')
                audit.session.send('Input.insertText', {'text': 'Ada Δ'})
                audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===6')
                assert 'Ada Δ' in audit.texts()
                audit.session.send('Input.insertText', {'text': '!7'})
                audit.page.wait_for_function('(' + TEXTS + ')().join(" ").includes("Regex rejected;")')
                assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 6
                assert 'Ada Δ' in audit.texts()
                audit.page.keyboard.press('Control+a')
                audit.session.send('Input.imeSetComposition', {'text': 'Ω', 'selectionStart': 1, 'selectionEnd': 1})
                audit.page.wait_for_function('(' + TEXTS + ')().includes("Ω")')
                assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 6
                audit.session.send('Input.imeSetComposition', {'text': '7', 'selectionStart': 1, 'selectionEnd': 1})
                audit.page.wait_for_function('(' + TEXTS + ')().join(" ").includes("Regex rejected;")')
                assert 'Ω' in audit.texts()
                assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 6
                audit.session.send('Input.insertText', {'text': 'Ω'})
                audit.page.wait_for_function('cuiGallery.exports.gallery_input_selection(2)===2')
                audit.page.keyboard.press('Control+z')
                audit.page.wait_for_function('(' + TEXTS + ')().includes("Ada Δ")')
                assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 6
                audit.page.keyboard.press('Control+y')
                audit.page.wait_for_function('(' + TEXTS + ')().includes("Ω")')
                assert audit.page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 2
                assert audit.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                assert not audit.errors, audit.errors
                audit.page.close()
            browser.close()
    finally:
        linked.shutdown()
        default.shutdown()
    print('PASS headless WASI and actual optional gallery Unicode typing/rejection/IME/Undo/Redo desktop/mobile and no-archive disabled mode')


if __name__ == '__main__':
    main()
