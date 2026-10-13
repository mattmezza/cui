#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Strict native Chromium CDP IME audit for Numeric, OTP and Combobox.

Collect all failures before exiting; unsupported consumers remain failing regressions.
Use --directory for the integrator's built gallery. No synthetic composition events.
"""
import argparse
import json
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    failures, observations = [], []
    def check(condition, story, requirement, detail=None):
        if not condition:
            failures.append({'story': story, 'requirement': requirement, 'detail': detail})
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900})
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.add_init_script('''window.imeAudit=[];for(const type of ['compositionstart','compositionupdate','compositionend'])
                document.addEventListener(type,e=>window.imeAudit.push({type,data:e.data}),true);''')
            session = page.context.new_cdp_session(page)
            for story, preedit, committed in [('NumberInput', '57', 57), ('OtpInput', '7', 1), ('Combobox', 'ap', 2)]:
                page.goto(f'http://127.0.0.1:{server.server_port}/#{story}')
                page.wait_for_function('(s)=>window.cuiGallery?.renderedComponent===s', arg=story)
                page.wait_for_timeout(100)
                box = page.evaluate('''()=>{const e=cuiGallery.exports,r=document.querySelector('canvas').getBoundingClientRect();
                    for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===6&&!e.gallery_semantic(i,5))
                        return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}''')
                check(bool(box), story, 'enabled textbox semantics')
                if not box:
                    continue
                page.mouse.click(box['x'], box['y'])
                page.wait_for_function('document.activeElement.id==="cui-ime-input"')
                value = lambda: page.evaluate('''()=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_semantic_count();i++)
                    if(e.gallery_semantic(i,0)===6)return e.gallery_semantic(i,8);}''')
                rendered = lambda: page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>
                    new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))).join('');}''')
                if story != 'OtpInput':
                    page.keyboard.press('Control+a')
                before = value()
                session.send('Input.imeSetComposition', {'text': preedit, 'selectionStart': len(preedit), 'selectionEnd': len(preedit)})
                page.wait_for_timeout(80)
                check(value() == before, story, 'preedit does not mutate value', [before, value()])
                check(preedit in rendered(), story, 'preedit visibly rendered', rendered())
                caret = page.evaluate('cuiGallery.exports.gallery_ime_rect(0)')
                check(caret >= 0, story, 'host candidate caret available', caret)
                session.send('Input.insertText', {'text': preedit})
                page.wait_for_timeout(80)
                after = value()
                check(after == committed, story, 'commit applied exactly once', [committed, after])
                # Cancel a different preedit; preserve the committed value.
                session.send('Input.imeSetComposition', {'text': '9', 'selectionStart': 1, 'selectionEnd': 1})
                page.wait_for_timeout(50)
                session.send('Input.imeSetComposition', {'text': '', 'selectionStart': 0, 'selectionEnd': 0})
                page.wait_for_timeout(50)
                check(value() == after, story, 'cancel preserves document', [after, value()])
                check('9' not in rendered(), story, 'cancel removes preedit', rendered())
                if story in ['Combobox', 'NumberInput']:
                    page.keyboard.press('Control+z')
                    page.wait_for_timeout(80)
                    check(value() == before, story, 'single undo restores value', [before, value()])
                if story == 'OtpInput':
                    page.locator('#otp-mask').check()
                    page.wait_for_timeout(50)
                    page.mouse.click(box['x'], box['y'])
                    page.wait_for_function('document.activeElement.id==="cui-ime-input"')
                    session.send('Input.imeSetComposition', {'text': '８９', 'selectionStart': 1, 'selectionEnd': 2})
                    page.wait_for_timeout(80)
                    commands = rendered()
                    check('８' not in commands and '９' not in commands and '８９' not in commands,
                          story, 'masked preedit never exposes digits', commands)
                    check(commands.count('*') == 2, story, 'masked first-scalar preedit reuses committed mask', commands)
                    check(value() == 1, story, 'masked preedit keeps committed count')
                    session.send('Input.insertText', {'text': '８９'})
                    page.wait_for_timeout(80)
                    check(value() == 2, story, 'fullwidth commit normalizes first scalar once', value())
                # Disabling a live native composition must cancel without changing data.
                session.send('Input.imeSetComposition', {'text': '8', 'selectionStart': 1, 'selectionEnd': 1})
                page.wait_for_timeout(50)
                disabled_before = value()
                page.locator('#disabled').check()
                page.wait_for_timeout(80)
                check(value() == disabled_before, story, 'disable cancels without commit')
                check(page.evaluate('document.activeElement.id') != 'cui-ime-input', story, 'disabled textbox releases bridge')
                events = page.evaluate('window.imeAudit')
                check(sum(e['type'] == 'compositionstart' for e in events) >= 3, story, 'native composition starts dispatched', events)
                check(any(e['type'] == 'compositionend' and e['data'] == preedit for e in events), story, 'native commit dispatched', events)
                check(any(e['type'] == 'compositionend' and not e['data'] for e in events), story, 'native cancel dispatched', events)
                observations.append({'story': story, 'before': before, 'after_commit': after, 'caret_x': caret, 'native_events': events})
            check(not errors, 'all', 'no browser runtime errors', errors)
            browser.close()
    finally:
        server.shutdown()
    print(json.dumps({'observations': observations, 'failures': failures}, indent=2))
    assert not failures, f'{len(failures)} IME requirements failed; see report above'


if __name__ == '__main__':
    main()
