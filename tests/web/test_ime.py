#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Chromium native CDP IME composition against every built-in text consumer."""
import argparse
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
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width':1280,'height':900})
            errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(f'http://127.0.0.1:{server.server_port}/#Input')
            session=page.context.new_cdp_session(page)
            for story in ['Input','Textarea','Editor','Questionnaire']:
                page.evaluate('(story)=>location.hash=story',story)
                page.wait_for_function('(story)=>window.cuiGallery?.renderedComponent===story',arg=story)
                page.wait_for_timeout(100)
                if story=='Questionnaire':
                    choice=page.evaluate("""()=>{const e=cuiGallery.exports,r=document.querySelector('canvas').getBoundingClientRect();
                        for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===4)
                            return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}""")
                    page.mouse.click(choice['x'],choice['y'])
                    page.wait_for_function('cuiGallery.exports.gallery_questionnaire_stats(0)===1')
                    page.wait_for_timeout(150)
                box=page.evaluate('''()=>{const e=cuiGallery.exports;
                    for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===6&&!e.gallery_semantic(i,5)){
                        const r=document.querySelector('canvas').getBoundingClientRect();
                        return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}}
                ''')
                assert box, story
                page.mouse.click(box['x'],box['y'])
                page.wait_for_function('document.activeElement.id==="cui-ime-input"')
                page.keyboard.press('Control+a')
                length=lambda:page.evaluate('''()=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_semantic_count();i++)
                    if(e.gallery_semantic(i,0)===6&&e.gallery_semantic(i,7))return e.gallery_semantic(i,8);}''')
                before=length()
                session.send('Input.imeSetComposition',{'text':'😀é','selectionStart':2,'selectionEnd':3})
                page.wait_for_timeout(80)
                assert length()==before,(story,'preedit mutated document',before,length())
                rendered=page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>
                    new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i))));}''')
                assert '😀é' in ''.join(rendered),(story,rendered,page.evaluate('({error:cuiGallery.exports.gallery_error(),editor:[0,1,2].map(i=>cuiGallery.exports.gallery_editor_stat(i)),active:document.activeElement.id})'))
                session.send('Input.insertText',{'text':'😀é'})
                page.wait_for_timeout(80)
                assert length()==6,(story,'commit duplicated',length())
                page.keyboard.press('Control+z')
                page.wait_for_timeout(80)
                assert length()==before,(story,'undo did not restore',length(),before)
                session.send('Input.imeSetComposition',{'text':'取消','selectionStart':2,'selectionEnd':2})
                page.wait_for_timeout(50)
                session.send('Input.imeSetComposition',{'text':'','selectionStart':0,'selectionEnd':0})
                page.wait_for_timeout(50)
                assert length()==before,(story,'cancel mutated document')
            page.evaluate('location.hash="Input"')
            page.wait_for_function('cuiGallery.renderedComponent==="Input"')
            page.wait_for_timeout(100)
            page.locator('#input-mode').select_option('1')
            page.wait_for_timeout(100)
            box=page.evaluate("""()=>{const e=cuiGallery.exports,r=document.querySelector('canvas').getBoundingClientRect();
                for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===6)
                    return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}""")
            page.mouse.click(box['x'],box['y'])
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            readonly_before=page.evaluate('cuiGallery.exports.gallery_input_selection(2)')
            session.send('Input.imeSetComposition',{'text':'読み','selectionStart':2,'selectionEnd':2})
            session.send('Input.insertText',{'text':'読み'})
            page.wait_for_timeout(80)
            assert page.evaluate('cuiGallery.exports.gallery_input_selection(2)')==readonly_before
            page.locator('#input-mode').select_option('0')
            page.wait_for_timeout(80)
            page.mouse.click(box['x'],box['y'])
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            session.send('Input.imeSetComposition',{'text':'cancel on blur','selectionStart':3,'selectionEnd':3})
            page.wait_for_timeout(50)
            page.locator('#disabled').check()
            page.wait_for_timeout(80)
            assert page.evaluate('cuiGallery.exports.gallery_input_selection(2)')==readonly_before
            assert not errors,errors
            browser.close()
    finally:
        server.shutdown()
    print('Native browser IME preedit/commit/cancel/undo passed for Input, Textarea, Editor and Questionnaire.')

if __name__=='__main__':
    main()
