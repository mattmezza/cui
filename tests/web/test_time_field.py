#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Focused real-WASM localized TimeField checks, independent of other stories."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from threading import Thread

from playwright.sync_api import sync_playwright



def set_candidate(cdp, value):
    cdp.send('Input.imeSetComposition',
             {'text': value, 'selectionStart': len(value), 'selectionEnd': len(value)})


def wait_for_paint(page, value, present=True):
    page.wait_for_function('''([value,present])=>{
        const e=cuiGallery.exports, d=new TextDecoder();
        const found=Array.from({length:e.gallery_count()},(_,i)=>d.decode(
            new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
                           e.gallery_command_length(i)))).includes(value);
        return found===present;
    }''', arg=[value, present])


def ordered_segment(page, name, segment):
    return page.evaluate('''([name,segment])=>{
      const e=cuiGallery.exports,d=new TextDecoder();
      const label=i=>d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),
          e.gallery_semantic_length(i)));
      for(let i=0;i<e.gallery_semantic_count();i++)
        if(e.gallery_semantic(i,0)===9&&label(i)===name){
          const id=e.gallery_semantic_id(i);
          for(let j=0;j<e.gallery_semantic_count();j++)
            if(e.gallery_semantic_parent(j)===id&&label(j)===segment){
              const n=e.gallery_semantic_id(j);
              return {owner:[Number(n&0xffffffffn),Number((n>>32n)&0xffffffffn)],
                  bounds:[1,2,3,4].map(field=>e.gallery_semantic(j,field)),
                  value:e.gallery_semantic(j,8)};
            }
        }
      throw Error('Missing TimeField segment: '+name+' '+segment);
    }''', [name, segment])


def ordered_tab_preview(page):
    canvas = page.locator('#preview')
    mode = page.locator('#time-field-tab-mode')
    mode.select_option('ordered')
    page.wait_for_function('document.querySelector("#snippet").textContent.includes("schedule_frame")')
    roots = page.locator('#accessible [role=group]')
    assert roots.evaluate_all('(groups)=>groups.map(g=>g.getAttribute("aria-label"))') == [
        'Start time', 'End time',
    ]
    assert 'cui_begin_time_field_frame_v1' in page.locator('#snippet').text_content()

    def focused(name):
        page.wait_for_function('''name=>{
          const target=name==='Save schedule'?'preview':'cui-ime-input';
          if(document.activeElement.id!==target)return false;
          const e=cuiGallery.exports,d=new TextDecoder();
          for(let i=0;i<e.gallery_semantic_count();i++)
            if(e.gallery_semantic(i,7)&&d.decode(new Uint8Array(
                e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                return true;
          return false;
        }''', arg=name)

    def value(name, segment):
        return ordered_segment(page, name, segment)['value']

    def digit_then_tab(digit, field, destination):
        # Use real key events while holding the async frame. The digit must
        # change the Tab outcome even though no preceding paint has run.
        page.evaluate('''()=>{
          const queued=[],raf=window.requestAnimationFrame;
          window.requestAnimationFrame=callback=>queued.push(callback);
          window.restoreTimeFieldFrame=()=>{
            window.requestAnimationFrame=raf;
            for(const callback of queued)raf(callback);
          };
        }''')
        try:
            page.keyboard.press(digit)
            assert value(field, 'Hour') != int(digit)
            page.keyboard.press('Tab')
            focused(destination)
            assert value(field, 'Hour') == int(digit)
        finally:
            page.evaluate('window.restoreTimeFieldFrame()')

    canvas.focus()
    page.keyboard.press('Tab')
    focused('Start time')
    digit_then_tab('3', 'Start time', 'End time')
    page.keyboard.press('Shift+Tab')
    focused('Start time')
    page.keyboard.press('ArrowUp')
    page.keyboard.press('Shift+Tab')
    focused('Start time')
    assert value('Start time', 'Hour') == 3 and value('Start time', 'Minute') == 31
    page.keyboard.press('Shift+Tab')
    page.wait_for_function('document.activeElement.id==="disabled"')

    page.keyboard.press('Tab')
    page.wait_for_function('document.activeElement.id==="preview"')
    page.keyboard.press('Tab')
    focused('Start time')
    page.keyboard.press('Tab')
    focused('Start time')
    page.keyboard.press('Tab')
    focused('End time')
    digit_then_tab('4', 'End time', 'Save schedule')
    page.keyboard.press('Enter')
    page.locator('#accessible [role=status][aria-label="Schedule saved."]').wait_for(state='attached')
    page.keyboard.press('Tab')
    page.wait_for_function('document.activeElement.matches(".component-mode a")')

    page.keyboard.press('Shift+Tab')
    page.wait_for_function('document.activeElement.id==="preview"')
    page.keyboard.press('Shift+Tab')
    focused('Save schedule')
    page.locator('#disabled').check()
    page.locator('#accessible [aria-label="Start time"] [role=textbox][disabled]').first.wait_for(
        state='attached')
    canvas.focus()
    page.keyboard.press('Tab')
    page.wait_for_function('document.activeElement.matches(".component-mode a")')
    assert page.locator('#cui-ime-input').count() == 0


def ordered_ime_preview(page):
    page.locator('#time-field-tab-mode').select_option('ordered')
    page.wait_for_function('document.querySelector("#snippet").textContent.includes("schedule_frame")')
    assert 'CUI_TIME_FIELD_IME_V1' in page.locator('#snippet').text_content()
    page.locator('#preview').focus()
    page.keyboard.press('Tab')
    page.wait_for_function('document.activeElement.id==="cui-ime-input"')
    cdp = page.context.new_cdp_session(page)

    def config():
        return page.evaluate('[1,2,3,4].map(i=>cuiGallery.exports.gallery_text_config(i)>>>0)')

    def assert_anchor(segment):
        assert config()[:2] == segment['owner']
        x, y, w, h = segment['bounds']
        actual = page.locator('#cui-ime-input').bounding_box()
        canvas = page.locator('#preview').bounding_box()
        assert x <= actual['x'] - canvas['x'] < x + w
        assert y <= actual['y'] - canvas['y'] < y + h

    hour = ordered_segment(page, 'Start time', 'Hour')
    assert_anchor(hour)
    page.evaluate('window.oldTimeProxy=document.activeElement')
    initial = config()
    set_candidate(cdp, '03')
    wait_for_paint(page, '03')
    assert ordered_segment(page, 'Start time', 'Hour')['value'] == 9
    cdp.send('Input.insertText', {'text': '03'})
    minute = ordered_segment(page, 'Start time', 'Minute')
    page.wait_for_function('''([lo,hi])=>{
        const e=cuiGallery.exports;
        return (e.gallery_text_config(1)>>>0)===lo && (e.gallery_text_config(2)>>>0)===hi;
    }''', arg=minute['owner'])
    assert ordered_segment(page, 'Start time', 'Hour')['value'] == 3
    assert config()[2:] != initial[2:]
    assert_anchor(minute)
    assert page.evaluate('''config=>{
        const e=cuiGallery.exports;
        return e.gallery_owned_text(0,0,0,0,0,...config)===0;
    }''', initial)

    set_candidate(cdp, '99')
    wait_for_paint(page, '99')
    cdp.send('Input.insertText', {'text': '99'})
    wait_for_paint(page, '99', present=False)
    assert ordered_segment(page, 'Start time', 'Minute')['value'] == 30

    set_candidate(cdp, '42')
    wait_for_paint(page, '42')
    page.evaluate('''()=>{
        const old=window.oldTimeProxy;
        if(old.isConnected)throw Error('Old IME proxy is still attached');
        for(const type of ['compositionstart','compositionupdate','compositionend'])
            old.dispatchEvent(new CompositionEvent(type,{data:'55'}));
        old.dispatchEvent(new InputEvent('input',{data:'55'}));
        old.dispatchEvent(new FocusEvent('blur'));
    }''')
    set_candidate(cdp, '')
    wait_for_paint(page, '42', present=False)
    assert_anchor(minute)
    assert ordered_segment(page, 'Start time', 'Minute')['value'] == 30

    cdp.send('Input.insertText', {'text': '41'})
    wait_for_paint(page, '41')
    assert ordered_segment(page, 'Start time', 'Minute')['value'] == 41
    assert ordered_segment(page, 'End time', 'Minute')['value'] == 45


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium')
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            return super().translate_path(path[4:] if path.startswith('/cui/') else path)

        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(('127.0.0.1', 0),
                                partial(Handler, directory=str(args.directory.resolve())))
    Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as playwright:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = playwright.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, device_scale_factor=2)
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            page.locator('canvas').scroll_into_view_if_needed()
            assert 'CUI_TIME_FIELD_IME_V1' in page.locator('#snippet').text_content()

            def box(name, role):
                return page.evaluate('''([name,role])=>{
                    const e=cuiGallery.exports,d=new TextDecoder();
                    for(let i=0;i<e.gallery_semantic_count();i++)
                        if(e.gallery_semantic(i,0)===role&&d.decode(new Uint8Array(
                            e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name)
                            return [1,2,3,4].map(field=>e.gallery_semantic(i,field));
                    throw Error('Missing semantic '+name);
                }''', [name, role])

            def next_paint():
                page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')

            def click(name, role):
                x, y, w, h = box(name, role)
                canvas = page.locator('canvas').bounding_box()
                page.mouse.click(canvas['x'] + x + w / 2, canvas['y'] + y + h / 2)
                next_paint()

            def stats(field):
                return page.evaluate('field=>cuiGallery.exports.gallery_control_stats(field)', field)

            def textbox(name, attribute=None, value=None):
                page.wait_for_function('''([name,attribute,value])=>{
                    const node=Array.from(document.querySelectorAll('#accessible [role=textbox]'))
                        .find(node=>node.getAttribute('aria-label')===name);
                    return node&&(!attribute||node.getAttribute(attribute)===value);
                }''', arg=[name, attribute, value])

            textbox('Hour')
            size = page.locator('#design-size')
            assert size.is_visible() and size.input_value() == '3'
            assert box('Time', 9)[3] == 32
            for value, expected, name in [
                ('1', 20, 'XS'), ('9', 80, '4XL'), ('14', 264, '9XL'), ('3', 32, 'BASE')
            ]:
                size.select_option(value)
                next_paint()
                root = box('Time', 9)
                cell = box('Hour', 6)
                assert root[3] == expected, (value, root)
                assert abs(root[1] + root[3] / 2 - cell[1] - cell[3] / 2) < 1
                assert root[0] + root[2] < page.locator('canvas').bounding_box()['width']
                assert f'CUI_DESIGN_{name}' in page.locator('#snippet').text_content()
            density = page.locator('#density')
            density.select_option('1')
            next_paint()
            assert box('Time', 9)[3] == 28
            density.select_option('0')
            next_paint()
            click('Hour', 6)
            page.keyboard.press('ArrowUp')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(16)===103000')
            callbacks = stats(17)
            assert callbacks == 1
            click('12-hour', 2)
            textbox('AM/PM')
            click('Arabic', 2)
            textbox('ساعة')
            click('Read only', 2)
            textbox('ساعة', 'aria-readonly', 'true')
            click('ساعة', 6)
            page.keyboard.press('ArrowUp')
            page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')
            assert stats(16) == 103000 and stats(17) == callbacks
            click('Read only', 2)
            click('12-hour', 2)
            click('Bounds', 2)
            click('ساعة', 6)
            page.keyboard.press('Home')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===5')
            assert stats(16) == 103000 and stats(17) == callbacks
            click('Clear validation', 1)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===0')
            click('دقيقة', 6)
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            cdp = page.context.new_cdp_session(page)

            set_candidate(cdp, '٢٠')
            wait_for_paint(page, '٢٠')
            assert stats(16) == 103000 and stats(17) == callbacks
            cdp.send('Input.insertText', {'text': '٢٠'})
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(16)===102000')
            assert stats(17) == callbacks + 1

            set_candidate(cdp, '٦٠')
            wait_for_paint(page, '٦٠')
            cdp.send('Input.insertText', {'text': '٦٠'})
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(18)===4')
            assert stats(16) == 102000 and stats(17) == callbacks + 1

            set_candidate(cdp, '٤٢')
            wait_for_paint(page, '٤٢')
            set_candidate(cdp, '')
            wait_for_paint(page, '٤٢', present=False)
            assert stats(16) == 102000 and stats(17) == callbacks + 1
            assert not errors, errors
            page.close()

            page = browser.new_page(viewport={'width': 1280, 'height': 900})
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            ordered_tab_preview(page)
            assert not errors, errors
            page.close()

            page = browser.new_page(viewport={'width': 1280, 'height': 900})
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            ordered_ime_preview(page)
            assert not errors, errors
            page.close()

            page = browser.new_page(viewport={'width': 390, 'height': 880}, device_scale_factor=2,
                                    is_mobile=True, has_touch=True)
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/#TimeField')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="TimeField"')
            stage = page.locator('#video-player-stage')
            stage.scroll_into_view_if_needed()
            x, y, w, h = box('Hour', 6)
            canvas = page.locator('canvas').bounding_box()
            page.touchscreen.tap(canvas['x'] + x + w / 2, canvas['y'] + y + h / 2)
            page.wait_for_function('document.activeElement.id==="cui-ime-input"')
            page.locator('#design-size').select_option('14')
            next_paint()
            stage.scroll_into_view_if_needed()
            rectangle = stage.bounding_box()
            x, y = round(rectangle['x'] + rectangle['width'] - 35), round(rectangle['y'] + 120)
            cdp = page.context.new_cdp_session(page)
            cdp.send('Input.dispatchTouchEvent', {'type': 'touchStart',
                                                  'touchPoints': [{'x': x, 'y': y}]})
            for offset in range(25, 226, 25):
                cdp.send('Input.dispatchTouchEvent', {
                    'type': 'touchMove', 'touchPoints': [{'x': x - offset, 'y': y}]})
            cdp.send('Input.dispatchTouchEvent', {'type': 'touchEnd', 'touchPoints': []})
            page.wait_for_function('document.getElementById("video-player-stage").scrollLeft > 80')
            page.locator('#design-size').select_option('3')
            page.locator('#time-field-tab-mode').select_option('ordered')
            page.wait_for_function('document.querySelector("#snippet").textContent.includes("schedule_frame")')
            stage.scroll_into_view_if_needed()
            x, y, w, h = box('Minute', 6)
            canvas = page.locator('#preview').bounding_box()
            page.touchscreen.tap(canvas['x'] + x + w / 2, canvas['y'] + y + h / 2)
            next_paint()
            assert page.evaluate('document.activeElement.id==="cui-ime-input"')
            page.keyboard.press('ArrowUp')
            next_paint()
            assert page.evaluate('''()=>{
                const e=cuiGallery.exports,d=new TextDecoder();
                for(let i=0;i<e.gallery_semantic_count();i++)
                    if(e.gallery_semantic(i,0)===6&&d.decode(new Uint8Array(
                        e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==='Minute')
                        return e.gallery_semantic(i,8)===31;
                return false;
            }''')
            assert not errors, errors
            browser.close()
            print('PASS: TimeField sizes, density, touch, keyboard, ordered Tab, H12/Arabic locale, and IME')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
