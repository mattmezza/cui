#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Chromium IME composition, Input resize and Textarea soft-wrap navigation."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def check_input_resize(page, session):
    page.keyboard.press('Control+a')
    session.send('Input.insertText', {'text': 'W' * 120})
    page.wait_for_function('cuiGallery.exports.gallery_input_selection(1) === 120')

    def sample_resizes(text_length):
        return page.evaluate("""(textLength) => {
            const canvas = document.querySelector('canvas');
            const previousWidth = canvas.style.width;
            const gallery = cuiGallery, e = gallery.exports;
            try {
                return [320, 224, 360, 230, 308].map(width => {
                    canvas.style.width = width + 'px';
                    const before = gallery.frame;
                    gallery.render();
                    const clips = [];
                    let textX;
                    for (let i = 0; i < e.gallery_count(); i++) {
                        const type = e.gallery_command(i, 0);
                        if (type === 3)
                            clips.push([e.gallery_command(i, 1), e.gallery_command(i, 3)]);
                        if (type === 2 && e.gallery_command_length(i) === textLength)
                            textX = e.gallery_command(i, 1);
                    }
                    return {width, frameCount: gallery.frame - before, clips,
                            caret: [e.gallery_ime_rect(0), e.gallery_ime_rect(2)], textX};
                });
            } finally {
                canvas.style.width = previousWidth;
                gallery.render();
            }
        }""", text_length)

    for preedit in (False, True):
        if preedit:
            session.send('Input.imeSetComposition', {'text': 'WWWW', 'selectionStart': 4,
                                                     'selectionEnd': 4})
            page.wait_for_function("""() => {
                const e = cuiGallery.exports;
                return Array.from({length: e.gallery_count()}, (_, i) =>
                    e.gallery_command_length(i)).includes(124);
            }""")
        samples = sample_resizes(124 if preedit else 120)
        caret_offset = samples[0]['caret'][0] - samples[0]['textX']
        for observed in samples:
            assert observed['frameCount'] == 1 and len(observed['clips']) >= 2, observed
            assert observed['clips'][0][1] == min(280, observed['width'] - 48), observed
            x, width = observed['clips'][1]
            caret_x, caret_width = observed['caret']
            assert width > 0 and caret_width == 1, observed
            assert x - .02 <= caret_x and caret_x + caret_width <= x + width + .02, observed
            assert abs(caret_x - observed['textX'] - caret_offset) < .02, observed
            if not preedit:
                assert abs((caret_x + caret_width) - (x + width)) < .02, observed
    session.send('Input.imeSetComposition', {'text': '', 'selectionStart': 0, 'selectionEnd': 0})
    assert page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == 120


def check_textarea_word_movement(page, session):
    length = 120
    stats = 'cuiGallery.exports.gallery_textarea_stats'
    page.keyboard.press('Control+a')
    session.send('Input.insertText', {'text': 'W' * length})
    page.wait_for_function(f'(length) => {stats}(0) === length && {stats}(1) > 2',
                           arg=length, timeout=3000)
    page.keyboard.press('Control+Home')
    page.wait_for_function(f'{stats}(2) === 0', timeout=3000)
    page.keyboard.press('End')
    page.wait_for_function(f'(length) => {stats}(2) > 0 && {stats}(2) < length',
                           arg=length, timeout=3000)
    wrap_byte = page.evaluate(f'{stats}(2)')
    page.keyboard.press('Alt+Shift+ArrowRight')
    page.wait_for_function(
        f'([length, wrap]) => {stats}(2) === length && {stats}(3) === wrap',
        arg=[length, wrap_byte], timeout=3000,
    )

    page.keyboard.press('Control+Home')
    page.wait_for_function(f'{stats}(2) === 0 && {stats}(4) === 0', timeout=3000)
    page.keyboard.press('End')
    page.wait_for_function(f'(wrap) => {stats}(2) === wrap', arg=wrap_byte, timeout=3000)
    first_row_y = page.evaluate('cuiGallery.exports.gallery_ime_rect(1)')
    assert first_row_y >= 0
    page.keyboard.press('ArrowRight')
    page.wait_for_function(
        f'([wrap, rowY]) => {stats}(2) === wrap && cuiGallery.exports.gallery_ime_rect(1) > rowY',
        arg=[wrap_byte, first_row_y], timeout=3000,
    )
    page.keyboard.press('Alt+ArrowLeft')
    page.wait_for_function(f'{stats}(2) === 0 && {stats}(3) === 0', timeout=3000)


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
                if story == 'Input':
                    check_input_resize(page, session)
                elif story == 'Textarea':
                    check_textarea_word_movement(page, session)
            page.evaluate('location.hash="Input"')
            page.wait_for_function('cuiGallery.renderedComponent==="Input"')
            page.wait_for_timeout(100)
            page.locator('#input-mode').select_option('1')
            page.wait_for_timeout(100)
            box=page.evaluate("""()=>{const e=cuiGallery.exports,r=document.querySelector('canvas').getBoundingClientRect();
                for(let i=0;i<e.gallery_semantic_count();i++)if(e.gallery_semantic(i,0)===6)
                    return {x:r.left+e.gallery_semantic(i,1)+8,y:r.top+e.gallery_semantic(i,2)+8};}""")
            page.mouse.click(box['x'],box['y'])
            page.wait_for_function('document.activeElement.id==="preview"&&cuiGallery.exports.gallery_text_config(0)===0')
            assert page.locator('#cui-ime-input').count() == 0
            readonly_before=page.evaluate('cuiGallery.exports.gallery_input_selection(2)')
            # The finalized readonly configuration stops host text input. A late
            # host composition must still be rejected by the actual C reducer.
            page.evaluate("""()=>{const e=cuiGallery.exports,b=new TextEncoder().encode('読み'),p=e.malloc(b.length);
                new Uint8Array(e.memory.buffer,p,b.length).set(b);
                e.gallery_composition(0,0,0,0,0);e.gallery_composition(1,p,b.length,2,2);
                e.gallery_composition(2,p,b.length,2,2);e.free(p);}""")
            page.mouse.move(box['x']+1, box['y']+1)
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
    print('Growing Input caret and IME preedit stayed aligned on each of five resize frames.')
    print('Textarea soft-wrap caret placement and Alt+Arrow word selection passed.')

if __name__=='__main__':
    main()
