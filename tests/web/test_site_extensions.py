#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Exercise per-family guides, bitmap effects and composed app in real Chromium."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--chromium')
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            return super().translate_path(path[4:] if path.startswith('/cui/') else path)
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width':1280,'height':1000}, device_scale_factor=2, permissions=['clipboard-read', 'clipboard-write'])
            errors=[]
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/')
            page.wait_for_function('window.cuiGallery?.catalog.components.length===78')
            page.locator('#home-page a[href="#docs/native-app-starter"]').first.click()
            page.wait_for_function('document.querySelector("#docs-title").textContent==="Build your first native CUI app"')
            content=page.locator('#docs-content').inner_text()
            for required in ['SDL3', 'Python 3', 'CMake', 'Ninja', '-std=c11', 'pkg-config --static --libs cui-sdl3', 'account menu', '--font']:
                assert required in content,required
            assert page.locator('#docs-content img').count()==3
            for image in page.locator('#docs-content img').all():
                image.scroll_into_view_if_needed()
                image.evaluate('(i)=>i.decode()')
                assert image.evaluate('(i)=>i.complete&&i.naturalWidth>0')
            compile_block=page.locator('#docs-content pre').filter(has_text='cc -std=c11')
            compile_block.locator('button.docs-copy').click()
            page.wait_for_function('Array.from(document.querySelectorAll("#docs-content button.docs-copy")).some(b=>b.textContent==="Copied")')
            assert 'pkg-config --static --libs cui-sdl3' in page.evaluate('navigator.clipboard.readText()')
            names=page.evaluate('cuiGallery.catalog.components.map(c=>c.name)')
            def story(name):
                page.evaluate('(name)=>location.hash=name',name)
                page.wait_for_function('(name)=>cuiGallery.renderedComponent===name',arg=name)
            def change(name,value):
                before=page.evaluate('([name,value])=>{const e=document.getElementById(name);e.value=String(value);e.dispatchEvent(new Event("change",{bubbles:true}));return cuiGallery.frame;}',[name,value])
                page.wait_for_function('(before)=>cuiGallery.frame>before',arg=before)
            for name in names:
                story(name)
                assert page.locator('#component-state').inner_text().strip(),name
                assert page.locator('#component-events').inner_text().strip(),name
                assert page.locator('#component-guides a').count()>0,name
            story('Theme')
            assert page.locator('#effect-blur').is_visible()
            assert page.evaluate('cuiGallery.exports.gallery_effect_pixel(50,20,0)')==235
            change('effect-blur',8)
            assert page.evaluate('cuiGallery.exports.gallery_effect_pixel(50,20,0)')<220
            change('effect-opacity',.5)
            assert page.evaluate('cuiGallery.exports.gallery_effect_pixel(50,20,3)')==128
            change('effect-blur',0)
            change('effect-angle',90)
            assert page.evaluate('cuiGallery.exports.gallery_effect_pixel(0,0,0)')==12
            story('TextView')
            change('motion-duration',.7)
            change('motion-stagger',.08)
            change('motion-easing',1)
            stats=page.evaluate('Array.from({length:3},(_,i)=>cuiGallery.exports.gallery_text_view_motion_stat(i))')
            assert abs(stats[0]-.7)<.001 and abs(stats[1]-.08)<.001 and stats[2]==1,stats
            page.locator('#text-view-fade').check()
            page.wait_for_timeout(60)
            def append_update():
                point=page.evaluate("""()=>{const e=cuiGallery.exports,d=new TextDecoder();for(let i=0;i<e.gallery_semantic_count();i++){if(d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==='Append update')return {x:e.gallery_semantic(i,1)+e.gallery_semantic(i,3)/2,y:e.gallery_semantic(i,2)+e.gallery_semantic(i,4)/2};}throw Error('append missing');}""")
                preview=page.locator('canvas');preview.scroll_into_view_if_needed();bounds=preview.bounding_box()
                before=page.evaluate('cuiGallery.exports.gallery_text_view_stats(0)')
                page.mouse.click(bounds['x']+point['x'],bounds['y']+point['y'])
                page.wait_for_function('(before)=>cuiGallery.exports.gallery_text_view_stats(0)>before',arg=before)
            append_update()
            opacity=page.evaluate('cuiGallery.exports.gallery_text_view_motion_stat(3)')
            assert 0<=opacity<1,opacity
            page.wait_for_function('cuiGallery.exports.gallery_text_view_motion_stat(3)===1')
            page.emulate_media(reduced_motion='reduce')
            append_update()
            assert page.evaluate('cuiGallery.exports.gallery_text_view_motion_stat(3)')==1
            page.emulate_media(reduced_motion='no-preference')
            story('Theme')
            widths=[]
            page.evaluate("""()=>{const option=document.createElement('option');option.value='4294967295';option.textContent='Unknown family fallback';document.getElementById('text-family').append(option);}""")
            for family in (0,1,2,3,4294967295):
                change('text-family',family)
                sample=page.evaluate("""()=>{const e=cuiGallery.exports;const d=new TextDecoder();for(let i=0;i<e.gallery_count();i++){if(e.gallery_command(i,0)===2&&d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==='WiiW: Build with CUI')return {family:e.gallery_command_family(i)>>>0,width:e.gallery_command(i,3),size:e.gallery_command(i,11)};}return null;}""")
                assert sample and sample['family']==family and sample['size']==24,sample
                widths.append(round(sample['width'],2))
            assert len(set(widths))>=3 and widths[0]==widths[-1],widths
            change('text-family',3)
            change('text-weight',3)
            change('text-italic',0) # Checkbox has its own interaction below.
            page.locator('#text-italic').check()
            page.wait_for_timeout(60)
            change('text-size',128)
            assert page.evaluate("""()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>e.gallery_command_family(i)===3&&e.gallery_command(i,11)===128&&e.gallery_command(i,22)===3&&e.gallery_command(i,23)===1);}""")
            page.evaluate('location.hash="examples/northstar"')
            page.wait_for_function('cuiGallery.renderedComponent==="Northstar workspace"')
            assert page.locator('#preview-card').is_visible()
            def click_id(identifier):
                if identifier==112 and page.evaluate('''()=>{const e=cuiGallery.exports;return !Array.from({length:e.gallery_semantic_count()},(_,i)=>i).some(i=>Number(e.gallery_semantic_id(i))===112);}'''):
                    click_id(100)
                canvas=page.locator('canvas')
                canvas.scroll_into_view_if_needed()
                bounds=canvas.bounding_box()
                lookup="""id=>{const e=cuiGallery.exports;const index=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>Number(e.gallery_semantic_id(i))===id);if(index===undefined)return null;return {x:e.gallery_semantic(index,1)+e.gallery_semantic(index,3)/2,y:e.gallery_semantic(index,2)+e.gallery_semantic(index,4)/2};}"""
                point=None
                for attempt in range(12):
                    point=page.evaluate(lookup,identifier)
                    if point and 0<point['y']<bounds['height']-8:
                        break
                    page.mouse.move(bounds['x']+bounds['width']*.7,bounds['y']+bounds['height']*.6)
                    page.mouse.wheel(0,200 if not point or point['y']>=bounds['height'] else -200)
                    page.wait_for_timeout(60)
                assert point and 0<point['y']<bounds['height'],(identifier,point,bounds)
                page.mouse.click(bounds['x']+point['x'],bounds['y']+point['y'])
                page.wait_for_timeout(40)
            click_id(120)
            page.wait_for_function('cuiGallery.exports.gallery_app_stat(0)===1')
            click_id(131)
            page.wait_for_function('cuiGallery.exports.gallery_app_stat(1)===1')
            click_id(101)
            click_id(102)
            page.wait_for_function('cuiGallery.exports.gallery_app_stat(2)===1')
            click_id(112)
            page.wait_for_function('cuiGallery.exports.gallery_app_stat(0)===2')
            click_id(140)
            page.keyboard.press('Control+a')
            page.keyboard.insert_text('Grace Hopper')
            click_id(143)
            page.wait_for_function('cuiGallery.exports.gallery_app_stat(3)===1')
            for theme in ('0','1','2'):
                change('theme',theme)
                assert page.evaluate('cuiGallery.exports.gallery_app_stat(3)')==1
            page.set_viewport_size({'width':390,'height':1000})
            page.wait_for_timeout(100)
            assert page.evaluate('document.documentElement.scrollWidth<=innerWidth')
            assert not errors,errors
            baseline = set(page.evaluate('cuiGallery.catalog.components.map(c => c.name)'))
            extensions = set(page.evaluate('(cuiGallery.catalog.extensions || []).map(c => c.name)'))
            assert len(baseline) == 78 and extensions == {'Video', 'VideoPlayer'} and not baseline & extensions
            assert set(page.evaluate('Object.keys(cuiGallery.stories)')) == baseline | extensions
            print('PASS:78 baseline usage guides plus Video extension, real gradient/blur/opacity pixels, font families/scales/styles, configurable/reduced motion, composed app CTA/account/edit/save/themes/mobile')
            browser.close()
    finally:
        server.shutdown()

if __name__=='__main__':
    main()
