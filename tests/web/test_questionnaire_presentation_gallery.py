#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real named text/error presentation with routed validation and navigation."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_questionnaire_progress_gallery import compile_usage


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
                assert a.page.locator('#questionnaire-presentation').input_value() == '0'
                assert 'Choose one direction to get started.' in a.texts()

                def stats():
                    return a.page.evaluate('[3,4,5,6].map(i=>cuiGallery.exports.gallery_questionnaire_stats(i))')

                def commands():
                    return a.page.evaluate('''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>({text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))),f:Array.from({length:12},(_,f)=>e.gallery_command(i,f))}));}''')

                def click(name, role=None):
                    target = a.target(name, role)
                    if mobile:
                        a.tap(target)
                    else:
                        point = a.point(target)
                        a.page.mouse.click(point['x'], point['y'])
                    a.page.wait_for_timeout(100)

                before = stats()
                for mode in range(1, 5):
                    a.configure('questionnaire-presentation', mode)
                    assert stats() == before
                    compile_usage(a.page.locator('#snippet').inner_text())
                    text = ' '.join(a.texts())
                    assert 'Portable desktop UI' in text
                    if mode >= 2:
                        assert 'Wählen Sie eine Richtung' in text
                    draws = commands()
                    width = a.page.locator('canvas').evaluate('(c)=>c.getBoundingClientRect().width')
                    localized = 'Wählen Sie eine Richtung für Ihr nächstes Projekt. Die Beschreibung bleibt auch auf einem schmalen Bildschirm vollständig lesbar.'
                    named_draws = [c for c in draws if c['text'] and
                                   (c['text'] in localized or c['text'] in ('What would you like to do?', 'Portable desktop UI'))]
                    assert named_draws
                    for c in named_draws:
                        assert c['f'][1] >= -1 and c['f'][1] + c['f'][3] <= width + 1
                    if mode == 2:
                        secondary = next(c for c in draws if c['text'] == 'Portable desktop UI')
                        # Gallery defaults to shared BASE: proportional secondary line24*.875.
                        assert secondary['f'][11] == 14 and secondary['f'][4] == 21, secondary
                    if mode == 3:
                        title = next(c for c in commands() if c['text'] == 'What would you like to do?')
                        assert title['f'][11] == 17 and title['f'][4] == 26
                # Validation comes from a real navigation button, not an injected error.
                click('Next')
                error = a.target('Bitte wählen Sie eine Antwort.')
                alert_role, alert_id = error['f'][0], error['id']
                assert len([n for n in a.snapshot() if n['f'][0] == alert_role]) == 1
                assert 'Bitte prüfen Sie Ihre Auswahl.' in a.texts()
                assert any(c['f'][0] == 0 and abs(c['f'][3] - 88) < .1 and
                           abs(c['f'][4] - 3) < .1 for c in commands())
                after_validation = stats()
                assert after_validation == before
                for mode in (2, 3, 4, 1, 0, 4):
                    a.configure('questionnaire-presentation', mode)
                    name = 'This question is required' if mode == 0 else 'Bitte wählen Sie eine Antwort.'
                    node = a.target(name, alert_role)
                    assert node['id'] == alert_id
                    assert len([n for n in a.snapshot() if n['f'][0] == alert_role]) == 1
                    assert stats() == after_validation
                    width = a.page.locator('canvas').evaluate('(c)=>c.getBoundingClientRect().width')
                    assert node['f'][1] >= -1 and node['f'][1] + node['f'][3] <= width + 1
                # Existing progress selector remains independent of named text.
                a.configure('questionnaire-progress', 2)
                assert a.target('Frage 1 von 2')['f'][8] == 1
                assert stats() == before
                click('Explore components', 4)
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===1')
                assert a.target('Frage 2 von 2')['f'][8] == 2
                assert not [n for n in a.snapshot() if n['f'][0] == alert_role]
                assert a.page.evaluate('cuiGallery.exports.gallery_questionnaire_stats(4)') == 1
                click('Previous')
                a.page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===0')
                assert a.target('Explore components', 4)['f'][6] == 1
                assert a.page.evaluate('document.documentElement.scrollWidth<=innerWidth+1')
                assert not a.errors, a.errors
                a.page.close()
                print(f'PASS named text/error modes, snippets, real validation/navigation; touch={mobile}', flush=True)
            browser.close()
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
