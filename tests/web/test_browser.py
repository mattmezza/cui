#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Chromium tests of the CUI WASM gallery, including repo-relative hosting."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    parser.add_argument('--chromium', help='Optional installed Chromium executable')
    args = parser.parse_args()
    root = args.directory.resolve()
    # Mount as /cui/ to exercise the same URL shape as GitHub project Pages.
    class Handler(SimpleHTTPRequestHandler):
        def translate_path(self, path):
            if path.startswith('/cui/'):
                path = path[4:]
            return super().translate_path(path)
        def log_message(self, *args):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(root)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 900}, device_scale_factor=2, permissions=['clipboard-read', 'clipboard-write'])
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            page.goto(f'http://127.0.0.1:{server.server_port}/cui/')
            page.wait_for_function('window.cuiGallery && window.cuiGallery.catalog.components.length === 78')
            assert len(page.evaluate('Object.keys(cuiGallery.stories)')) == 70
            assert page.locator('#home-page').is_visible()
            assert page.locator('#gallery-page').is_hidden()
            assert 'Portable C11 UI' in page.title()
            assert page.locator('#home-story-count').inner_text() == str(len(page.evaluate('Object.keys(cuiGallery.stories)')))
            page.screenshot(path='/tmp/cui-web-home-light.png')
            page.emulate_media(color_scheme='dark')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="dark"')
            page.screenshot(path='/tmp/cui-web-home-dark.png')
            page.emulate_media(color_scheme='light')
            page.set_viewport_size({'width':390,'height':844})
            assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'), 'Homepage overflows mobile viewport'
            page.locator('#menu').click()
            assert page.locator('#sidebar').is_visible()
            page.set_viewport_size({'width':1280,'height':900})
            page.locator('#copy-quickstart').click()
            page.wait_for_function('document.querySelector("#copy-quickstart").textContent==="Copied"')
            assert '#include "cui/cui_components.h"' in page.evaluate('navigator.clipboard.readText()')
            page.evaluate('location.hash="#docs/getting-started"')
            page.wait_for_function('document.querySelector("#docs-page")&&!document.querySelector("#docs-page").hidden')
            assert page.locator('#docs-content h1').first.inner_text() == 'CUI'
            assert page.locator('#docs-content a[href="#docs/api"]').count() >= 1
            assert page.locator('#docs-nav a').count() >= 40
            page.wait_for_function('document.querySelector("#docs-content img")?.complete&&document.querySelector("#docs-content img")?.naturalWidth>0')
            page.wait_for_function('window.scrollY===0')
            page.screenshot(path='/tmp/cui-web-docs.png')
            page.evaluate('location.hash="#docs/getting-started/build-and-run"')
            page.wait_for_function('document.querySelector("#doc-build-and-run")?.getBoundingClientRect().top<200&&window.scrollY>0')
            page.locator('#docs-content a[href="#docs/api"]').first.click()
            page.wait_for_function('location.hash==="#docs/api"&&document.querySelector("#docs-title").textContent.includes("API")')
            page.keyboard.press('/')
            assert page.locator('#search').evaluate('(node)=>document.activeElement===node')
            page.locator('#search').fill('bt')
            assert page.locator('#catalog a').first.get_attribute('href') == '#Button'
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.renderedComponent==="Button"&&cuiGallery.exports.gallery_count()>0')
            assert page.locator('#gallery-page').is_visible()
            def control_change(control, value):
                before = page.evaluate('({control,value})=>{const node=document.getElementById(control);if(node.type==="checkbox")node.checked=value;else node.value=String(value);node.dispatchEvent(new Event("change",{bubbles:true}));return cuiGallery.frame;}', {'control':control,'value':value})
                page.wait_for_function('(before)=>cuiGallery.frame>before', arg=before)
            assert page.locator('#theme').input_value() == '3'
            assert page.evaluate('getComputedStyle(document.body).colorScheme') == 'light'
            light_canvas = page.evaluate('Array.from({length:3},(_,i)=>cuiGallery.exports.gallery_command(0,i+5))')
            page.emulate_media(color_scheme='dark')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="dark"&&cuiGallery.exports.gallery_command(0,0)===0')
            page.wait_for_function('(light)=>Array.from({length:3},(_,i)=>cuiGallery.exports.gallery_command(0,i+5)).some((value,i)=>value!==light[i])', arg=light_canvas)
            control_change('theme', '0')
            assert page.evaluate('getComputedStyle(document.body).colorScheme') == 'light'
            page.wait_for_function('(light)=>Array.from({length:3},(_,i)=>cuiGallery.exports.gallery_command(0,i+5)).every((value,i)=>value===light[i])', arg=light_canvas)
            page.emulate_media(color_scheme='light')
            page.emulate_media(color_scheme='dark')
            assert page.evaluate('getComputedStyle(document.body).colorScheme') == 'light'
            control_change('theme', '3')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="dark"')
            page.emulate_media(color_scheme='light')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="light"')
            page.locator('#palette-trigger').click()
            page.locator('#palette-menu [data-palette="high-contrast"]').click()
            assert page.evaluate('getComputedStyle(document.body).backgroundColor') == 'rgb(255, 255, 255)'
            page.locator('#palette-trigger').click()
            page.locator('#palette-menu [data-palette="system"]').click()
            # The picker uses the same pinned palette data for CSS and the C draw theme.
            presets = page.evaluate('cuiGallery.themes')
            assert len(presets) == 38
            assert page.locator('#palette-menu [role=group]').count() == 4
            assert page.locator('#palette-menu [role=option]').count() == 40
            for index, preset in enumerate(presets):
                page.locator('#palette-trigger').click()
                assert page.locator('#palette-menu .palette-check:visible').count() == 1
                if index == 0:
                    page.screenshot(path='/tmp/cui-web-palette-light.png')
                before_palette_frame = page.evaluate('cuiGallery.frame')
                page.locator(f'#palette-menu [data-palette="{preset["id"]}"]').click()
                page.wait_for_function('(expected)=>cuiGallery.themeChoice===expected.id&&getComputedStyle(document.body).backgroundColor===expected.css', arg={'id':preset['id'],'css':f'rgb({int(preset["colors"]["background"][1:3],16)}, {int(preset["colors"]["background"][3:5],16)}, {int(preset["colors"]["background"][5:7],16)})'})
                page.wait_for_function('(before)=>cuiGallery.frame>before', arg=before_palette_frame)
                channels = page.evaluate('Array.from({length:4},(_,channel)=>cuiGallery.exports.gallery_theme_channel(0,channel))')
                assert channels == [int(preset['colors']['background'][n:n+2],16) for n in (1,3,5,7)], (index,preset['name'],channels)
                assert page.locator('#palette-trigger').get_attribute('aria-expanded') == 'false'
                assert page.locator('#palette-trigger').evaluate('(node)=>document.activeElement===node')
                if index == 12:
                    page.locator('#palette-trigger').click()
                    page.screenshot(path='/tmp/cui-web-palette-dark.png')
                    page.keyboard.press('Escape')
            page.reload()
            page.wait_for_function('window.cuiGallery&&cuiGallery.themeChoice===localStorage.getItem("cui.theme")')
            assert page.locator('#palette-label').inner_text() == presets[-1]['name']
            page.emulate_media(color_scheme='dark')
            page.emulate_media(color_scheme='light')
            assert page.evaluate('getComputedStyle(document.body).colorScheme') == 'dark'
            page.locator('#palette-trigger').click()
            page.keyboard.press('Home')
            assert page.locator('#palette-menu [data-palette="system"]').evaluate('(node)=>document.activeElement===node')
            page.keyboard.press('ArrowDown')
            assert page.locator('#palette-menu [data-palette="default-light"]').evaluate('(node)=>document.activeElement===node')
            assert page.locator('#palette-menu [tabindex="0"]').count() == 1
            page.keyboard.press('Escape')
            assert page.locator('#palette-trigger').evaluate('(node)=>document.activeElement===node')
            page.locator('#palette-trigger').click()
            page.keyboard.press('Home')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.themeChoice==="system"')
            page.locator('#palette-trigger').click()
            page.mouse.click(700, 250)
            assert page.locator('#palette-menu').is_hidden()
            page.emulate_media(color_scheme='dark')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="dark"')
            page.emulate_media(color_scheme='light')
            page.wait_for_function('getComputedStyle(document.body).colorScheme==="light"')
            assert page.request.get(f'http://127.0.0.1:{server.server_port}/cui/LICENSE.txt').ok
            assert page.locator('#catalog a').count() == 78
            assert page.locator('#name').inner_text() == 'Button'
            assert page.locator('#accessible [role=link][aria-label="Link"]').count() == 1
            page.locator('#copy-snippet').click()
            page.wait_for_function('document.querySelector("#copy-snippet").textContent==="Copied"')
            assert page.evaluate('navigator.clipboard.readText()') == page.locator('#snippet').inner_text()
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 0
            assert page.locator('#accessible [aria-busy=true]').count() == 1
            canvas = page.locator('canvas')
            box = canvas.bounding_box()
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2, button='right')
            page.wait_for_timeout(50)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 0
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function('cuiGallery.exports.gallery_clicks() === 1')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.exports.gallery_clicks() === 2')
            control_change('disabled', True)
            page.wait_for_function('cuiGallery.exports.gallery_semantic(0,5) === 1')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_timeout(80)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 2
            height = page.evaluate('cuiGallery.exports.gallery_semantic(0,4)')
            control_change('density', '1')
            page.wait_for_function('(height)=>cuiGallery.exports.gallery_semantic(0,4)<height', arg=height)
            control_change('density', '0')
            control_change('disabled', False)
            def button_text_geometry():
                return page.evaluate('''()=>{
                    const e=cuiGallery.exports,c=document.querySelector('canvas'),ctx=c.getContext('2d'),scale=c.width/c.clientWidth;
                    const bounds=[1,2,3,4].map(f=>e.gallery_semantic(0,f));
                    const text=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==='Run action');
                    if(text===undefined)return null;
                    const x=e.gallery_command(text,1),w=e.gallery_command(text,3),font=e.gallery_command(text,11);
                    let top=Infinity,bottom=-Infinity;
                    for(let py=Math.ceil(bounds[1]*scale);py<Math.floor((bounds[1]+bounds[3])*scale);py++){
                      const row=ctx.getImageData(Math.ceil((x+1)*scale),py,Math.floor((w-2)*scale),1).data;
                      const pixels=Array.from({length:row.length/4},(_,i)=>[row[i*4],row[i*4+1],row[i*4+2]]);
                      const bg=[0,1,2].map(ch=>pixels.map(p=>p[ch]).sort((a,b)=>a-b)[Math.floor(pixels.length/2)]);
                      const ink=pixels.some(p=>Math.abs(p[0]-bg[0])+Math.abs(p[1]-bg[1])+Math.abs(p[2]-bg[2])>120);
                      if(ink){top=Math.min(top,py/scale);bottom=Math.max(bottom,(py+1)/scale);}
                    }
                    return {height:bounds[3],font,center:(top+bottom)/2-(bounds[1]+bounds[3]/2),ink:bottom-top};
                }''')
            for theme in ('0', '1'):
                control_change('theme', theme)
                for size, expected_height, expected_font in ((3,20,12),(1,24,14),(0,32,14),(2,32,16)):
                    control_change('control-size', size)
                    geometry = button_text_geometry()
                    assert geometry and abs(geometry['height']-expected_height) < 0.3, (theme,size,geometry)
                    assert abs(geometry['font']-expected_font) < 0.1, (theme,size,geometry)
                    assert geometry['ink'] > 4 and abs(geometry['center']) < 2, (theme,size,geometry)
                page.screenshot(path=f'/tmp/cui-web-buttons-{"light" if theme == "0" else "dark"}.png')
            control_change('control-size', '0')
            assert page.locator('#features tr').count() > 0
            assert page.locator('#reference').get_attribute('href').startswith('https://gpui-kit.com/')
            control_change('theme', '1')
            assert page.locator('body.dark').count() == 1
            page.evaluate('location.hash="Theme"')
            page.wait_for_function('cuiGallery.renderedComponent==="Theme"')
            shape = page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).find(i=>{const e=cuiGallery.exports;return e.gallery_command(i,0)===0&&e.gallery_command(i,5)===29&&e.gallery_command(i,6)===100;})')
            assert shape is not None
            assert page.evaluate('(i)=>Array.from({length:5},(_,j)=>cuiGallery.exports.gallery_command(i,12+j))', shape) == [1,24,0,0,18]
            pixels = page.evaluate('''(i)=>{
                const e=cuiGallery.exports,c=document.querySelector('canvas'),ctx=c.getContext('2d');
                const x=e.gallery_command(i,1),y=e.gallery_command(i,2),scale=c.width/c.clientWidth;
                return [[60,32],[1,32],[116,32],[60,1],[60,62],[1,1]].map(([dx,dy])=>Array.from(ctx.getImageData(Math.floor((x+dx)*scale),Math.floor((y+dy)*scale),1,1).data));
            }''', shape)
            blue, orange = [29,100,220,255], [240,150,20,255]
            assert all(all(abs(a-b) <= 1 for a,b in zip(pixel,expected))
                       for pixel,expected in zip(pixels[:5],[blue,blue,orange,orange,blue])), pixels
            assert pixels[5] not in [blue,orange], pixels
            focusable_log = page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,0)===38&&cuiGallery.exports.gallery_semantic(i,33)===1)')
            passive_log = page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,0)===38&&cuiGallery.exports.gallery_semantic(i,33)===0)')
            assert focusable_log is not None and passive_log is not None
            assert page.locator('#accessible [role=log]').count() == 2
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,9)', passive_log) == -1
            theme_button = page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,0)===1)')
            b = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', theme_button)
            rect = canvas.bounding_box()
            page.mouse.click(rect['x']+b[0]+b[2]/2,rect['y']+b[1]+b[3]/2)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=theme_button)
            page.keyboard.press('Tab')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=focusable_log)
            page.keyboard.press('Tab')
            page.wait_for_function('document.activeElement!==document.querySelector("canvas")')
            for name in page.evaluate('Object.keys(cuiGallery.stories)'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(50)
                assert page.locator('#preview-card').is_visible(), name
                assert 'draw commands' in page.locator('#preview-note').inner_text(), (name, page.locator('#preview-note').inner_text(), errors)
            def semantic_index(name, role=None):
                return page.evaluate('({name,role})=>Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>{const e=cuiGallery.exports;return (role===null?e.gallery_semantic(i,0)!==27:e.gallery_semantic(i,0)===role)&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name;})', {'name':name,'role':role})
            def click_semantic(name, role=None):
                index = semantic_index(name, role)
                assert index is not None, name
                canvas.scroll_into_view_if_needed()
                bounds = page.evaluate('(index)=>Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(index,i+1))', index)
                page.evaluate('y=>{const rect=document.querySelector("canvas").getBoundingClientRect(),header=document.querySelector("header").getBoundingClientRect(),target=rect.top+y;if(target<header.bottom+8)window.scrollBy(0,target-header.bottom-8);}', bounds[1]+bounds[3]/2)
                rectangle = canvas.bounding_box()
                assert page.evaluate('({x,y})=>document.elementFromPoint(x,y)===document.querySelector("canvas")', {'x':rectangle['x']+bounds[0]+bounds[2]/2,'y':rectangle['y']+bounds[1]+bounds[3]/2}), name
                page.mouse.click(rectangle['x']+bounds[0]+bounds[2]/2,rectangle['y']+bounds[1]+bounds[3]/2)
            def dropdown_select(control, value, role=None):
                control_change('dropdown-'+control, value)
                if role is not None:
                    page.wait_for_function('(role)=>Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===role&&new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_semantic_name(i),cuiGallery.exports.gallery_semantic_length(i)))==="Export options")', arg=role)
            def dropdown_check(control):
                control_change('dropdown-'+control, True)
            page.evaluate('location.hash="Sidebar"')
            page.wait_for_function('cuiGallery.renderedComponent==="Sidebar"')
            assert semantic_index('Workspace sidebar', 9) is not None
            assert semantic_index('Overview', 23) is not None
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_command(i,0)===5)')
            page.screenshot(path='/tmp/cui-web-sidebar-expanded.png')
            click_semantic('Files', 23)
            page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(3)===1&&cuiGallery.exports.gallery_sidebar_stats(4)===1')
            click_semantic('Toggle sidebar', 1)
            page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(0)===1&&Math.abs(cuiGallery.exports.gallery_sidebar_stats(1)-48)<.2')
            assert page.locator('#sidebar-collapsed').is_checked()
            page.screenshot(path='/tmp/cui-web-sidebar-collapsed.png')
            control_change('sidebar-mode', '1')
            page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(1)===0&&cuiGallery.exports.gallery_sidebar_stats(5)===0')
            control_change('sidebar-mode', '2')
            page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(1)===250&&cuiGallery.exports.gallery_sidebar_stats(5)===1')
            control_change('sidebar-collapsed', False)
            control_change('sidebar-mode', '0')
            control_change('sidebar-side', '1')
            right_x = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('Workspace sidebar', 9))
            assert right_x > 300, right_x
            control_change('sidebar-side', '0')
            viewport = semantic_index('Scrollable content', 9)
            bounds = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', viewport)
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+bounds[0]+bounds[2]/2,rect['y']+bounds[1]+bounds[3]/2)
            page.mouse.wheel(0,240)
            page.wait_for_function('cuiGallery.exports.gallery_sidebar_stats(2)>0')
            page.evaluate('location.hash="Textarea"')
            page.wait_for_function('cuiGallery.renderedComponent==="Textarea"')
            assert page.locator('#accessible [role=textbox][aria-label="Project notes"][aria-multiline=true]').count() == 1
            page.screenshot(path='/tmp/cui-web-textarea.png')
            length = page.evaluate('cuiGallery.exports.gallery_textarea_stats(0)')
            click_semantic('Project notes', 6)
            page.keyboard.press('Control+End')
            page.keyboard.press('Enter')
            page.keyboard.type('more')
            page.wait_for_function('(length)=>cuiGallery.exports.gallery_textarea_stats(0)===length+5', arg=length)
            assert page.evaluate('()=>new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_textarea_text(),cuiGallery.exports.gallery_textarea_stats(0))).endsWith(String.fromCharCode(10)+"more")')
            control_change('textarea-readonly', True)
            page.keyboard.type('X')
            assert page.evaluate('cuiGallery.exports.gallery_textarea_stats(0)') == length+5
            control_change('textarea-readonly', False)
            control_change('textarea-rows', '1')
            normal_height = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Project notes', 6))
            control_change('textarea-grow', True)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Project notes', 6)) > normal_height
            wrapped = page.evaluate('cuiGallery.exports.gallery_textarea_stats(1)')
            control_change('textarea-nowrap', True)
            assert page.evaluate('cuiGallery.exports.gallery_textarea_stats(1)') < wrapped
            heights = []
            for size in (3,1,0,2):
                control_change('control-size', size)
                heights.append(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Project notes', 6)))
            assert heights[-1] > heights[0], heights
            for name in ('Checkbox','Radio','Switch','Toggle','Form','Input Group','OtpInput','Tag','Attachment'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>cuiGallery.renderedComponent===name', arg=name)
                assert page.locator('#control-size').is_visible(), name
                layouts = []
                for size in (3,1,0,2):
                    control_change('control-size', size)
                    layouts.append(page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).slice(0,40).map(i=>[0,1,2,3,4,11].map(f=>e.gallery_command(i,f)));}'''))
                assert layouts[0] != layouts[-1], name
                if name in ('Checkbox','Radio'):
                    control_change('control-size', '0')
                    for theme in ('0','1'):
                        control_change('theme', theme)
                        page.screenshot(path=f'/tmp/cui-web-{name.lower()}-{"light" if theme == "0" else "dark"}.png')
            for name, trigger, option in (('Select','Delivery','Standard'),):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>cuiGallery.renderedComponent===name', arg=name)
                heights=[]
                for size in (3,2):
                    control_change('control-size', size)
                    click_semantic(trigger)
                    page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
                    item=semantic_index(option)
                    assert item is not None, (name,size)
                    heights.append(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', item))
                    page.keyboard.press('Escape')
                    page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
                assert heights[1] > heights[0], (name,heights)
            page.evaluate('location.hash="DescriptionList"')
            page.wait_for_function('cuiGallery.renderedComponent==="DescriptionList"')
            owner = semantic_index('Owner', 7)
            team = semantic_index('Team', 7)
            assert owner is not None and team is not None
            owner_y = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,2)', owner)
            assert abs(owner_y-page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,2)', team)) < 1
            control_change('description-columns', '1')
            page.wait_for_function('()=>{const e=cuiGallery.exports,items=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).filter(i=>e.gallery_semantic(i,0)===7);return items.length===8&&e.gallery_semantic(items[2],2)>e.gallery_semantic(items[0],2);}')
            control_change('description-vertical', True)
            page.wait_for_function('()=>{const e=cuiGallery.exports,items=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).filter(i=>e.gallery_semantic(i,0)===7);return e.gallery_semantic(items[1],2)>e.gallery_semantic(items[0],2);}')
            base_height = page.evaluate('cuiGallery.exports.gallery_semantic(0,4)')
            control_change('description-size', '2')
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,4)') > base_height
            control_change('description-plain', True)
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,4)') > 0
            page.evaluate('location.hash="StatusBar"')
            page.wait_for_function('cuiGallery.renderedComponent==="StatusBar"')
            assert all(semantic_index(name, 7) is not None for name in ('main', 'gallery.c', 'Ln 42, Col 8'))
            center = semantic_index('gallery.c', 7)
            middle_x = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', center)
            control_change('status-right', False)
            assert semantic_index('Ln 42, Col 8', 7) is None
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('gallery.c', 7)) > middle_x
            control_change('status-custom', True)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_command(i,9)===8)')
            page.evaluate('location.hash="Resizable"')
            page.wait_for_function('cuiGallery.renderedComponent==="Resizable"')
            separator = page.locator('#accessible [role=separator][aria-label="Resize panels"]')
            assert separator.get_attribute('aria-orientation') == 'vertical'
            assert float(separator.get_attribute('aria-valuenow')) >= 80
            initial_panel = page.evaluate('cuiGallery.exports.gallery_resizable_size(0)')
            click_semantic('Resize panels', 40)
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(size)=>cuiGallery.exports.gallery_resizable_size(0)>size', arg=initial_panel)
            keyboard_panel = page.evaluate('cuiGallery.exports.gallery_resizable_size(0)')
            handle = semantic_index('Resize panels', 40)
            handle_bounds = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', handle)
            canvas_bounds = canvas.bounding_box()
            handle_x = canvas_bounds['x']+handle_bounds[0]+handle_bounds[2]/2
            handle_y = canvas_bounds['y']+handle_bounds[1]+handle_bounds[3]/2
            page.mouse.move(handle_x, handle_y)
            page.mouse.down()
            page.mouse.move(handle_x+24, handle_y, steps=4)
            page.mouse.up()
            page.wait_for_function('(size)=>cuiGallery.exports.gallery_resizable_size(0)>size+15', arg=keyboard_panel)
            control_change('resizable-disabled', True)
            locked_panel = page.evaluate('cuiGallery.exports.gallery_resizable_size(0)')
            page.keyboard.press('ArrowRight')
            assert page.evaluate('cuiGallery.exports.gallery_resizable_size(0)') == locked_panel
            control_change('resizable-disabled', False)
            control_change('resizable-vertical', True)
            assert separator.get_attribute('aria-orientation') == 'horizontal'
            assert abs(sum(page.evaluate('[0,1].map(i=>cuiGallery.exports.gallery_resizable_size(i))'))-210) < 1
            page.evaluate('location.hash="Icon"')
            page.wait_for_function('cuiGallery.renderedComponent==="Icon"')
            def icon_bitmap():
                return page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_command(i,0)===5)')
            bitmap = icon_bitmap()
            assert bitmap is not None
            assert page.evaluate('cuiGallery.exports.gallery_icon_stats(3)') == 0
            first_parse = page.evaluate('cuiGallery.exports.gallery_icon_stats(0)')
            control_change('icon-choice', '1')
            assert page.evaluate('cuiGallery.exports.gallery_icon_stats(0)') == first_parse+1
            assert icon_bitmap() is not None
            control_change('icon-size', '4')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_command(i,3)', icon_bitmap()) >= 24
            control_change('icon-color', True)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_bitmap(i,9)', icon_bitmap()) == 1
            tinted = page.evaluate('(i)=>[10,11,12,13].map(f=>cuiGallery.exports.gallery_bitmap(i,f))', icon_bitmap())
            assert tinted[3] == 255 and tinted[:3] != [0, 0, 0]
            control_change('icon-rotate', True)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_command(i,3)', icon_bitmap()) > 24
            assert page.locator('#accessible [role=img]').count() == 0
            page.evaluate('location.hash="Toolbar"')
            page.wait_for_function('cuiGallery.renderedComponent==="Toolbar"')
            toolbar = page.locator('#accessible [role=toolbar][aria-label="Editing toolbar"]')
            assert toolbar.get_attribute('aria-orientation') == 'horizontal'
            assert page.locator('#accessible [role=group][aria-label="Formatting"]').count() == 1
            assert all(semantic_index(name, 1) is not None for name in ('Save', 'Undo', 'Bold', 'Italic'))
            bar = semantic_index('Editing toolbar', 39)
            assert abs(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', bar)-48) < 1
            saved = page.evaluate('cuiGallery.exports.gallery_toolbar_activations(0)')
            click_semantic('Save', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_toolbar_activations(0)===n+1', arg=saved)
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('Undo', 1))
            control_change('toolbar-input', False)
            assert semantic_index('Find', 6) is None
            click_semantic('Italic', 1)
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('Save', 1))
            control_change('toolbar-grouped', False)
            assert page.locator('#accessible [role=group][aria-label="Formatting"]').count() == 0
            assert semantic_index('Bold', 1) is not None
            control_change('toolbar-size', '0')
            assert abs(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Editing toolbar', 39))-32) < 1
            control_change('toolbar-size', '1')
            assert abs(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Editing toolbar', 39))-28) < 1
            control_change('toolbar-size', '4')
            assert abs(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Editing toolbar', 39))-48) < 1
            control_change('toolbar-size', '2')
            control_change('toolbar-input', True)
            click_semantic('Find', 6)
            page.keyboard.press('Home')
            page.wait_for_function('cuiGallery.exports.gallery_toolbar_cursor()===0')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('cuiGallery.exports.gallery_toolbar_cursor()===1')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', semantic_index('Find', 6)) == 1
            control_change('disabled', True)
            assert page.locator('#accessible [role=button][aria-label="Save"]').is_disabled()
            control_change('disabled', False)
            page.evaluate('location.hash="TitleBar"')
            page.wait_for_function('cuiGallery.renderedComponent==="TitleBar"')
            title = semantic_index('Window title bar', 9)
            assert title is not None
            assert abs(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', title)-34) < 1
            assert page.locator('#accessible [role=button]').count() == 0
            gradient = page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_command(i,0)===5&&cuiGallery.exports.gallery_bitmap(i,1)===34)')
            assert gradient is not None
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_bitmap(i,0)', gradient) == 1
            control_change('title-decoration', '2')
            assert all(semantic_index(name, 1) is not None for name in ('Minimize window', 'Maximize window', 'Close window'))
            click_semantic('Minimize window', 1)
            page.wait_for_function('cuiGallery.exports.gallery_title_action(1)===1')
            click_semantic('Maximize window', 1)
            page.wait_for_function('cuiGallery.exports.gallery_title_action(2)===1&&cuiGallery.exports.gallery_title_action(8)===1')
            page.wait_for_function('document.querySelector(`#accessible [role=button][aria-label="Restore window"]`)!==null')
            click_semantic('Restore window', 1)
            page.wait_for_function('cuiGallery.exports.gallery_title_action(3)===1&&cuiGallery.exports.gallery_title_action(8)===0')
            close_actions = page.evaluate('cuiGallery.exports.gallery_title_action(4)')
            control_change('title-close', True)
            click_semantic('Close window', 1)
            page.wait_for_function('cuiGallery.exports.gallery_title_action(7)===1')
            assert page.evaluate('cuiGallery.exports.gallery_title_action(4)') == close_actions
            canvas.scroll_into_view_if_needed()
            page.evaluate('y=>{const rect=document.querySelector("canvas").getBoundingClientRect(),header=document.querySelector("header").getBoundingClientRect(),target=rect.top+y;if(target<header.bottom+8)window.scrollBy(0,target-header.bottom-8);}', 41)
            title_rect = canvas.bounding_box()
            blank_x, blank_y = title_rect['x']+350, title_rect['y']+41
            assert page.evaluate('({x,y})=>document.elementFromPoint(x,y)===document.querySelector("canvas")', {'x':blank_x,'y':blank_y})
            moves = page.evaluate('cuiGallery.exports.gallery_title_action(0)')
            page.mouse.move(blank_x, blank_y)
            page.mouse.down()
            page.mouse.move(blank_x+20, blank_y, steps=3)
            page.mouse.up()
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(0)===n+1', arg=moves)
            menus = page.evaluate('cuiGallery.exports.gallery_title_action(5)')
            page.mouse.click(blank_x, blank_y, button='right')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(5)===n+1', arg=menus)
            doubles = page.evaluate('cuiGallery.exports.gallery_title_action(6)')
            page.mouse.click(blank_x, blank_y)
            page.mouse.click(blank_x+1, blank_y)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_title_action(6)===n+1', arg=doubles)
            client_title_x = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('CUI workspace', 7))
            control_change('title-decoration', '3')
            assert page.locator('#accessible [role=button]').count() == 0
            native_title_x = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('CUI workspace', 7))
            assert native_title_x > client_title_x+60
            control_change('title-fullscreen', True)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('CUI workspace', 7)) > native_title_x+10
            control_change('title-decoration', '0')
            assert page.locator('#accessible [role=button]').count() == 0
            page.evaluate('location.hash="Notification"')
            page.wait_for_function('cuiGallery.renderedComponent==="Notification"')
            control_change('notification-autohide', False)
            close_count = page.evaluate('cuiGallery.exports.gallery_notification_stats(3)')
            click_semantic('Add notification', 1)
            page.wait_for_function('cuiGallery.exports.gallery_notification_stats(0)===1&&cuiGallery.exports.gallery_notification_entry(0,0)===1')
            def notification_bounds(index=0):
                return page.evaluate('(i)=>[2,3,4,5].map(f=>cuiGallery.exports.gallery_notification_entry(i,f))', index)
            for anchor in range(8):
                control_change('notification-anchor', str(anchor))
                page.wait_for_function('anchor=>{const e=cuiGallery.exports,c=document.querySelector("canvas"),x=e.gallery_notification_entry(0,2),y=e.gallery_notification_entry(0,3),w=e.gallery_notification_entry(0,4),h=e.gallery_notification_entry(0,5);const expectedX=[0,3,6].includes(anchor)?16:[1,4].includes(anchor)?(c.clientWidth-w)/2:c.clientWidth-w-16;const expectedY=anchor<3?164:anchor<6?c.clientHeight-h-16:(c.clientHeight-h)/2;return Math.abs(x-expectedX)<.2&&Math.abs(y-expectedY)<.2;}', arg=anchor)
            control_change('notification-anchor', '2')
            page.wait_for_function('cuiGallery.exports.gallery_notification_entry(0,3)===164')
            action_count = page.evaluate('cuiGallery.exports.gallery_notification_stats(2)')
            click_semantic('Review', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_notification_stats(2)===n+1', arg=action_count)
            assert page.evaluate('cuiGallery.exports.gallery_notification_stats(0)') == 1
            b = notification_bounds()
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+b[0]+18,rect['y']+b[1]+18)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Close"]`)!==null')
            click_semantic('Close', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_notification_stats(0)===0&&cuiGallery.exports.gallery_notification_stats(3)===n+1', arg=close_count)
            click_semantic('Add three alerts', 1)
            page.mouse.move(0,0)
            page.wait_for_function('cuiGallery.exports.gallery_notification_stats(0)===3&&[0,1,2].every(i=>cuiGallery.exports.gallery_notification_entry(i,0)===1)')
            newest = notification_bounds(2)
            oldest = notification_bounds(0)
            assert abs(oldest[1]-newest[1]-28) < 1, (oldest,newest)
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+newest[0]+18,rect['y']+newest[1]+18)
            page.wait_for_function('()=>{const e=cuiGallery.exports;return e.gallery_notification_entry(0,3)-e.gallery_notification_entry(2,3)>2*e.gallery_notification_entry(2,5)+20;}')
            assert page.locator('#accessible button[aria-label="Close"]').count() == 3
            click_semantic('Dismiss alerts', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_notification_stats(0)===0&&cuiGallery.exports.gallery_notification_stats(3)===n+4', arg=close_count)
            control_change('notification-autohide', True)
            click_semantic('Add notification', 1)
            page.wait_for_function('cuiGallery.exports.gallery_notification_entry(0,0)===1')
            b = notification_bounds()
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+b[0]+18,rect['y']+b[1]+18)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Close"]`)!==null')
            remaining = page.evaluate('cuiGallery.exports.gallery_notification_entry(0,1)')
            page.wait_for_timeout(600)
            assert abs(page.evaluate('cuiGallery.exports.gallery_notification_entry(0,1)')-remaining) < .02
            click_semantic('Pin alert', 2)
            pin_index = semantic_index('Pin alert', 2)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=pin_index)
            page.mouse.move(0,0)
            remaining = page.evaluate('cuiGallery.exports.gallery_notification_entry(0,1)')
            page.wait_for_timeout(600)
            assert abs(page.evaluate('cuiGallery.exports.gallery_notification_entry(0,1)')-remaining) < .02
            # Leaving the Canvas clears native focus and permits real five-second expiry.
            page.locator('#notification-anchor').focus()
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===0', arg=pin_index)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_notification_stats(0)===0&&cuiGallery.exports.gallery_notification_stats(3)===n+5', arg=close_count, timeout=10000)
            control_change('notification-autohide', False)
            click_count = page.evaluate('cuiGallery.exports.gallery_notification_stats(1)')
            for button in ['left','middle']:
                click_semantic('Add notification', 1)
                page.wait_for_function('cuiGallery.exports.gallery_notification_stats(0)===1&&cuiGallery.exports.gallery_notification_entry(0,0)===1')
                b = notification_bounds()
                rect = canvas.bounding_box()
                page.mouse.click(rect['x']+b[0]+18,rect['y']+b[1]+18,button=button)
                page.wait_for_function('cuiGallery.exports.gallery_notification_stats(0)===0')
            assert page.evaluate('cuiGallery.exports.gallery_notification_stats(1)') == click_count + 1
            assert page.evaluate('cuiGallery.exports.gallery_notification_stats(3)') == close_count + 7
            page.evaluate('location.hash="Shimmer"')
            page.wait_for_function('cuiGallery.renderedComponent==="Shimmer"')
            assert page.locator('#accessible [role=status][aria-label="Preparing your workspace"]').count() == 1
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()>0')
            control_change('shimmer-mode', '1')
            page.wait_for_function('cuiGallery.exports.gallery_shimmer_phase()>.8')
            phase = page.evaluate('cuiGallery.exports.gallery_shimmer_phase()')
            page.wait_for_function('(p)=>cuiGallery.exports.gallery_shimmer_phase()<p-.05', arg=phase)
            control_change('shimmer-mode', '2')
            page.wait_for_function('cuiGallery.exports.gallery_shimmer_phase()===1&&cuiGallery.exports.gallery_next_wake()===0&&cuiGallery.exports.gallery_redraw()===0')
            page.emulate_media(reduced_motion='reduce')
            control_change('shimmer-mode', '3')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()===0&&cuiGallery.exports.gallery_redraw()===0')
            page.emulate_media(reduced_motion='no-preference')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()>0')
            page.evaluate('location.hash="Message"')
            page.wait_for_function('cuiGallery.renderedComponent==="Message"')
            bitmap = page.evaluate('''()=>{const e=cuiGallery.exports,c=document.querySelector('canvas'),
                ctx=c.getContext('2d'),i=Array.from({length:e.gallery_count()},(_,i)=>i)
                .find(i=>e.gallery_command(i,0)===5),f=n=>e.gallery_command(i,n),scale=c.width/c.clientWidth;
                const pixel=(dx,dy)=>Array.from(ctx.getImageData(Math.round((f(1)+dx)*scale),
                    Math.round((f(2)+dy)*scale),1,1).data);
                return {source:Array.from({length:14},(_,n)=>e.gallery_bitmap(i,n)),
                    samples:[pixel(2,2),pixel(15,15),pixel(31,15),pixel(36,15)]};}''')
            assert bitmap['source'] == [4,4,20,1,1,2,2,.5,1,1,255,128,0,128]
            corner, center, clipped_edge, outside = bitmap['samples']
            assert all(abs(a-b) <= 1 for a,b in zip(corner,outside)) and all(abs(a-b) <= 2 for a,b in zip(center,clipped_edge)), bitmap
            assert center != corner and center[0] > center[1] > center[2] and center[3] == 255
            assert page.locator('#accessible [role=listitem][aria-label="Workspace conversation"]').count() == 1
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,9)', semantic_index('Workspace conversation', 37)) == -1
            click_semantic('Acknowledge message', 2)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,6)===1', arg=semantic_index('Acknowledge message', 2))
            actions = page.evaluate('cuiGallery.exports.gallery_message_actions()')
            click_semantic('Like message', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_message_actions()===n+1', arg=actions)
            def message_text_x(text):
                return page.evaluate('(text)=>{const e=cuiGallery.exports;let i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))===text);return e.gallery_command(i,1);}', text)
            control_change('message-avatar', False)
            control_change('message-inset', '2')
            start_x = message_text_x('Alex Morgan')
            control_change('message-inset', '1')
            assert abs(message_text_x('Alex Morgan')-start_x-12) < .2
            frame = page.evaluate('cuiGallery.frame')
            control_change('message-alignment', '1')
            page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
            assert message_text_x('Alex Morgan') > start_x+100
            for variant in map(str,range(7)):
                frame = page.evaluate('cuiGallery.frame')
                control_change('message-variant', variant)
                page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                assert page.evaluate('cuiGallery.exports.gallery_error()') == 0
            page.evaluate('location.hash="MessageScroller"')
            page.wait_for_function('cuiGallery.renderedComponent==="MessageScroller"&&cuiGallery.exports.gallery_conversation(3)===1')
            assert page.evaluate('cuiGallery.exports.gallery_conversation(0)') == 80
            assert 0 < page.evaluate('cuiGallery.exports.gallery_conversation(7)') < 32
            log = semantic_index('Conversation', 38)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,33)', log) == 1
            control_change('message-scrollbar', False)
            page.wait_for_function('!document.querySelector(`#accessible [role=scrollbar]`)')
            log = semantic_index('Conversation', 38)
            # Programmatic focus enters the real native Log; keyboard behavior remains in CUI.
            page.locator('#accessible [role=log]').focus()
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=log)
            page.keyboard.press('Home')
            page.wait_for_function('cuiGallery.exports.gallery_conversation(1)===0&&cuiGallery.exports.gallery_conversation(4)===1')
            page.wait_for_function('cuiGallery.exports.gallery_conversation(5)===1&&cuiGallery.exports.gallery_conversation(6)===1')
            scroll_before = page.evaluate('cuiGallery.exports.gallery_conversation(1)')
            click_semantic('Add message', 1)
            page.wait_for_function('cuiGallery.exports.gallery_conversation(0)===81')
            assert page.evaluate('cuiGallery.exports.gallery_conversation(3)') == 0
            assert abs(page.evaluate('cuiGallery.exports.gallery_conversation(1)')-scroll_before) < .2
            click_semantic('Jump to latest', 1)
            page.wait_for_function('cuiGallery.exports.gallery_conversation(3)===1&&cuiGallery.exports.gallery_conversation(4)===0')
            click_semantic('Add message', 1)
            page.wait_for_function('cuiGallery.exports.gallery_conversation(0)===82&&cuiGallery.exports.gallery_conversation(3)===1')
            page.locator('#accessible [role=log]').focus()
            page.keyboard.press('Home')
            page.wait_for_function('cuiGallery.exports.gallery_conversation(1)===0')
            page.keyboard.press('PageDown')
            page.wait_for_function('cuiGallery.exports.gallery_conversation(1)>0')
            scroll_before = page.evaluate('cuiGallery.exports.gallery_conversation(1)')
            visible_message = '''()=>{const e=cuiGallery.exports,log=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>e.gallery_semantic(i,0)===38&&e.gallery_semantic(i,33));
                const top=e.gallery_semantic(log,2),bottom=top+e.gallery_semantic(log,4);
                const row=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>e.gallery_semantic(i,0)===37&&e.gallery_semantic(i,2)>=top&&e.gallery_semantic(i,2)<bottom);
                return [new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(row),e.gallery_semantic_length(row))),e.gallery_semantic(row,2)-top];}'''
            anchored = page.evaluate(visible_message)
            click_semantic('Prepend earlier', 1)
            page.wait_for_function('(before)=>cuiGallery.exports.gallery_conversation(0)===83&&cuiGallery.exports.gallery_conversation(1)>before', arg=scroll_before)
            assert page.evaluate('cuiGallery.exports.gallery_conversation(1)') > scroll_before
            page.wait_for_function('({id,y})=>{const e=cuiGallery.exports,log=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>e.gallery_semantic(i,0)===38&&e.gallery_semantic(i,33)),top=e.gallery_semantic(log,2),bottom=top+e.gallery_semantic(log,4),row=Array.from({length:e.gallery_semantic_count()},(_,i)=>i).find(i=>e.gallery_semantic(i,0)===37&&e.gallery_semantic(i,2)>=top&&e.gallery_semantic(i,2)<bottom);return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(row),e.gallery_semantic_length(row)))===id&&Math.abs(e.gallery_semantic(row,2)-top-y)<=2;}', arg={'id': anchored[0], 'y': anchored[1]}, timeout=2000)
            settled = page.evaluate(visible_message)
            assert settled[0] == anchored[0] and abs(settled[1]-anchored[1]) <= 2, (anchored,settled)
            click_semantic('Remove first', 1)
            page.wait_for_function('cuiGallery.exports.gallery_conversation(0)===82')
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()===0&&cuiGallery.exports.gallery_redraw()===0')
            page.emulate_media(reduced_motion='no-preference')
            page.evaluate('location.hash="Image"')
            page.wait_for_function('cuiGallery.renderedComponent==="Image"&&cuiGallery.exports.gallery_image_stats(1)===1')
            image_command = lambda: page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_command(i,0)===5)')
            assert image_command() is not None
            page.wait_for_function('cuiGallery.exports.gallery_image_stats(3)===1', timeout=2000)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_bitmap_revision(i,0)', image_command()) == 2
            page.emulate_media(reduced_motion='reduce')
            control_change('image-state', '1')
            page.wait_for_function('cuiGallery.exports.gallery_image_stats(1)===0')
            control_change('image-state', '0')
            page.wait_for_function('()=>{const e=cuiGallery.exports,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===5);return e.gallery_image_stats(1)===1&&e.gallery_image_stats(3)===0&&i!==undefined&&e.gallery_bitmap_revision(i,0)===1;}')
            image_width = page.evaluate('(i)=>cuiGallery.exports.gallery_command(i,3)', image_command())
            control_change('image-fit', '1')
            page.wait_for_function('(w)=>{const e=cuiGallery.exports,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===5);return e.gallery_command(i,3)>w;}', arg=image_width)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_bitmap(i,6)', image_command()) < 4
            for fit in ('2','3','4','0'):
                control_change('image-fit', fit)
                page.wait_for_function('cuiGallery.exports.gallery_error()===0')
            control_change('image-grayscale', True)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_bitmap(i,8)===1', arg=image_command())
            def image_sample():
                return page.evaluate('''()=>{const e=cuiGallery.exports,c=document.querySelector('canvas'),ctx=c.getContext('2d'),i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===5),scale=c.width/c.clientWidth,x=e.gallery_command(i,1)+e.gallery_command(i,3)/2,y=e.gallery_command(i,2)+e.gallery_command(i,4)/2;return [Array.from(ctx.getImageData(Math.round(x*scale),Math.round(y*scale),1,1).data),Array.from(ctx.getImageData(2,2,1,1).data)];}''')
            opaque, background = image_sample()
            assert max(opaque[:3])-min(opaque[:3]) <= 2, opaque
            control_change('image-opacity', True)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_bitmap(i,7)===.5', arg=image_command())
            half, _ = image_sample()
            assert all(abs(value-(opaque[channel]+background[channel])/2)<=3 for channel,value in enumerate(half[:3])), (opaque,half,background)
            control_change('image-state', '1')
            page.wait_for_function('cuiGallery.exports.gallery_image_stats(1)===0&&cuiGallery.exports.gallery_image_stats(2)===1')
            assert page.locator('#accessible [aria-label="Workspace preview image"]').count() == 1
            control_change('image-state', '2')
            page.wait_for_function('cuiGallery.exports.gallery_image_stats(1)===2')
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_command_text(i),cuiGallery.exports.gallery_command_length(i)))==="Image failed")')
            page.emulate_media(reduced_motion='no-preference')
            page.evaluate('location.hash="Attachment"')
            page.wait_for_function('cuiGallery.renderedComponent==="Attachment"&&cuiGallery.exports.gallery_attachment_stats(5)>0')
            assert page.locator('#accessible [role=group]').evaluate_all('(nodes)=>nodes.filter(node=>/^Attachment [1-6]$/.test(node.getAttribute("aria-label"))).length') == 6
            actions = page.evaluate('cuiGallery.exports.gallery_attachment_stats(3)')
            click_semantic('Open', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_attachment_stats(3)===n+1', arg=actions)
            clicks = page.evaluate('cuiGallery.exports.gallery_attachment_stats(0)')
            click_semantic('Attachment 1', 9)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_attachment_stats(0)===n+1', arg=clicks)
            for status in ('0','1','2','3','4'):
                control_change('attachment-status', status)
                page.wait_for_function('(status)=>cuiGallery.exports.gallery_attachment_stats(9)===Number(status)', arg=status)
            control_change('attachment-vertical', True)
            page.wait_for_function('cuiGallery.exports.gallery_error()===0')
            control_change('attachment-vertical', False)
            control_change('attachment-status', '3')
            page.wait_for_function('document.querySelectorAll(`#accessible [aria-label="Retry attachment"]`).length>0')
            retries = page.evaluate('cuiGallery.exports.gallery_attachment_stats(2)')
            click_semantic('Retry attachment', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_attachment_stats(2)===n+1&&cuiGallery.exports.gallery_attachment_stats(9)===1', arg=retries)
            card = semantic_index('Attachment 1', 9)
            bounds = page.evaluate('(i)=>Array.from({length:4},(_,j)=>cuiGallery.exports.gallery_semantic(i,j+1))', card)
            canvas_box = canvas.bounding_box()
            page.mouse.move(canvas_box['x']+bounds[0]+bounds[2]/2,canvas_box['y']+bounds[1]+bounds[3]/2)
            page.wait_for_function('document.querySelector(`#accessible [aria-label="Remove attachment"]`)')
            removals = page.evaluate('cuiGallery.exports.gallery_attachment_stats(1)')
            click_semantic('Remove attachment', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_attachment_stats(1)===n+1&&cuiGallery.exports.gallery_attachment_stats(4)===1', arg=removals)
            page.wait_for_function('Array.from(document.querySelectorAll(`#accessible [role=group]`)).filter(node=>/^Attachment [1-6]$/.test(node.getAttribute("aria-label"))).length===5')
            page.mouse.move(canvas_box['x']+bounds[0]+60,canvas_box['y']+bounds[1]+40)
            page.mouse.wheel(400,0)
            page.wait_for_function('cuiGallery.exports.gallery_attachment_stats(6)>0')
            page.evaluate('location.hash="Accordion"')
            page.wait_for_function('cuiGallery.renderedComponent==="Accordion"')
            started = page.locator('#accessible button[aria-label="Getting started"]')
            config = page.locator('#accessible button[aria-label="Configuration"]')
            details = page.locator('#accessible button[aria-label="More details"]')
            assert started.get_attribute('aria-expanded') == 'true'
            click_semantic('Configuration', 1)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Configuration"]`)?.getAttribute("aria-expanded")==="true"')
            assert started.get_attribute('aria-expanded') == 'false'
            panel = page.locator('#accessible [role=group][aria-label="Configuration"]')
            assert panel.count() == 1
            assert config.get_attribute('aria-controls') == panel.get_attribute('id')
            assert panel.get_attribute('aria-labelledby') == config.get_attribute('id')
            page.keyboard.press('End')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('More details', 1))
            page.keyboard.press('Enter')
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="More details"]`)?.getAttribute("aria-expanded")==="true"')
            assert config.get_attribute('aria-expanded') == 'false'
            page.keyboard.press('Home')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('Getting started', 1))
            page.keyboard.press('Space')
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Getting started"]`)?.getAttribute("aria-expanded")==="true"')
            assert details.get_attribute('aria-expanded') == 'false'
            control_change('disabled', True)
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible button")).every(e=>e.disabled)')
            click_semantic('Getting started', 1)
            page.wait_for_timeout(40)
            assert started.get_attribute('aria-expanded') == 'true'
            control_change('disabled', False)
            page.evaluate('location.hash="Collapsible"')
            page.wait_for_function('cuiGallery.renderedComponent==="Collapsible"')
            trigger = page.locator('#accessible button[aria-label="Show details"]')
            assert trigger.get_attribute('aria-expanded') == 'false'
            click_semantic('Show details', 1)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Show details"]`)?.getAttribute("aria-expanded")==="true"')
            panel = page.locator('#accessible [role=group][aria-label="Show details"]')
            assert trigger.get_attribute('aria-controls') == panel.get_attribute('id')
            assert panel.get_attribute('aria-labelledby') == trigger.get_attribute('id')
            page.keyboard.press('Space')
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Show details"]`)?.getAttribute("aria-expanded")==="false"')
            assert panel.count() == 0
            control_change('disclosure-mode', '1')
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===0')
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)>0&&cuiGallery.exports.gallery_disclosure_progress(0)<1')
            click_semantic('Show details', 1)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Show details"]`)?.getAttribute("aria-expanded")==="false"')
            # Reversal uses the retained spring state rather than restarting a transition.
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===1')
            apply_index = semantic_index('Apply panel', 1)
            apply_bounds = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', apply_index)
            actions = page.evaluate('cuiGallery.exports.gallery_disclosure_actions()')
            click_semantic('Apply panel', 1)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_disclosure_actions()===n+1', arg=actions)
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)>0&&document.querySelector(`#accessible button[aria-label="Apply panel"]`)===null')
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            assert page.evaluate(texts+'.includes("Apply panel")')
            rect = canvas.bounding_box()
            frame = page.evaluate('cuiGallery.frame')
            page.mouse.click(rect['x']+apply_bounds[0]+apply_bounds[2]/2,rect['y']+apply_bounds[1]+apply_bounds[3]/2)
            page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
            assert page.evaluate('cuiGallery.exports.gallery_disclosure_actions()') == actions + 1
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===0&&cuiGallery.exports.gallery_redraw()===0')
            frame = page.evaluate('cuiGallery.frame')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.frame') == frame
            page.emulate_media(reduced_motion='reduce')
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===1')
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===0')
            assert page.locator('#accessible button[aria-label="Apply panel"]').count() == 0
            page.emulate_media(reduced_motion='no-preference')
            control_change('disclosure-mode', '2')
            page.wait_for_function(texts+'.includes(" (custom)")')
            click_semantic('Show details', 1)
            page.wait_for_function('cuiGallery.exports.gallery_disclosure_progress(0)===1')
            page.evaluate('location.hash="Accordion"')
            page.wait_for_function('cuiGallery.renderedComponent==="Accordion"')
            control_change('disclosure-mode', '2')
            control_change('disclosure-multiple', True)
            click_semantic('Configuration', 1)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Getting started"]`)?.getAttribute("aria-expanded")==="true"&&document.querySelector(`#accessible button[aria-label="Configuration"]`)?.getAttribute("aria-expanded")==="true"')
            assert page.locator('#accessible [role=group][aria-label="Getting started"]').count() == 1
            assert page.locator('#accessible [role=group][aria-label="Configuration"]').count() == 1
            control_change('theme', '0')
            page.evaluate('location.hash="Tag"')
            page.wait_for_function('cuiGallery.renderedComponent==="Tag"')
            page.mouse.move(0,0)
            tag_index = semantic_index('Workspace', 7)
            assert tag_index is not None
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,9)', tag_index) == -1
            assert page.locator('#accessible button[aria-label="Workspace"]').count() == 0
            assert page.locator('#tag-color option').count() == 21
            tag_draw = """({index,kind})=>{const e=cuiGallery.exports;for(let i=0;i<e.gallery_count();i++){if(e.gallery_command(i,0)!==kind)continue;if([1,2,3,4].every(f=>Math.abs(e.gallery_command(i,f)-e.gallery_semantic(index,f))<.01))return {rgba:[5,6,7,8].map(f=>e.gallery_command(i,f)),radius:e.gallery_command(i,13),height:e.gallery_command(i,4)};}return null;}"""
            control_change('tag-variant', '7')
            page.wait_for_function('args=>('+tag_draw+')(args)?.rgba.join(",")==="239,246,255,255"', arg={'index':tag_index,'kind':0})
            tag_bounds = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', tag_index)
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+tag_bounds[0]+tag_bounds[2]/2,rect['y']+tag_bounds[1]+tag_bounds[3]/2)
            page.wait_for_function('args=>('+tag_draw+')(args)?.rgba[3]===229', arg={'index':tag_index,'kind':0})
            page.mouse.move(0,0)
            control_change('tag-outline', True)
            page.wait_for_function('args=>('+tag_draw+')(args)===null', arg={'index':tag_index,'kind':0})
            assert page.evaluate(tag_draw, {'index':tag_index,'kind':1})['rgba'] == [191,219,254,255]
            control_change('tag-outline', False)
            control_change('tag-pill', True)
            page.wait_for_function('args=>{const c=('+tag_draw+')(args);return c&&c.radius>=c.height/2;}', arg={'index':tag_index,'kind':0})
            control_change('theme', '1')
            page.wait_for_function('args=>('+tag_draw+')(args)?.rgba.join(",")==="23,37,84,127"', arg={'index':tag_index,'kind':0})
            control_change('theme', '0')
            page.evaluate('location.hash="Marker"')
            page.wait_for_function('cuiGallery.renderedComponent==="Marker"')
            assert page.locator('#accessible [role=status][aria-label="Workspace synchronization"]').count() == 1
            marker_lines = '()=>Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).filter(i=>cuiGallery.exports.gallery_command(i,0)===0&&cuiGallery.exports.gallery_command(i,4)===1&&cuiGallery.exports.gallery_command(i,3)>20).length'
            control_change('marker-variant', '1')
            page.wait_for_function('('+marker_lines+')()===2')
            control_change('marker-alignment', '1')
            page.wait_for_function('('+marker_lines+')()===1')
            control_change('marker-variant', '2')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>{let e=cuiGallery.exports;return e.gallery_command(i,0)===1&&e.gallery_command(i,17)===1&&e.gallery_command(i,21)===1&&[18,19,20].every(f=>e.gallery_command(i,f)===0);})')
            control_change('marker-loading', '1')
            page.wait_for_function('document.querySelector(`#accessible [role=status][aria-label="Loading"]`)!==null')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()>0')
            control_change('marker-icon', True)
            page.wait_for_function('document.querySelector(`#accessible [role=status][aria-label="Loading"]`)===null&&cuiGallery.exports.gallery_next_wake()===0')
            control_change('marker-loading', '2')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()>0')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_command(i,0)===3)')
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()===0&&cuiGallery.exports.gallery_redraw()===0')
            frame = page.evaluate('cuiGallery.frame')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.frame') == frame
            page.emulate_media(reduced_motion='no-preference')
            control_change('marker-loading', '0')
            page.evaluate('location.hash="OtpInput"')
            page.wait_for_function('cuiGallery.renderedComponent==="OtpInput"')
            otp = page.locator('#accessible [role=textbox][aria-label="Verification code"]')
            assert otp.count() == 1
            def otp_cells():
                return page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).filter(i=>{const e=cuiGallery.exports;return e.gallery_command(i,0)===0&&e.gallery_command(i,3)===32&&e.gallery_command(i,4)===32;}).map(i=>cuiGallery.exports.gallery_command(i,1))')
            cells = otp_cells()
            assert len(cells) == 6, cells
            assert [round(cells[i+1]-cells[i]) for i in range(5)] == [36,36,52,36,36]
            control_change('otp-groups', '3')
            page.wait_for_timeout(40)
            cells = otp_cells()
            assert [round(cells[i+1]-cells[i]) for i in range(5)] == [36,52,36,52,36]
            click_semantic('Verification code', 6)
            page.wait_for_function('cuiGallery.exports.gallery_next_wake()>0')
            page.wait_for_timeout(80)
            visible = page.evaluate('cuiGallery.exports.gallery_otp_cursor()')
            frame = page.evaluate('cuiGallery.frame')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.frame') == frame
            page.wait_for_function('(v)=>cuiGallery.exports.gallery_otp_cursor()!==v', arg=visible)
            assert page.evaluate('cuiGallery.frame') <= frame + 3
            before = page.evaluate('cuiGallery.exports.gallery_otp_completions()')
            page.keyboard.type('a1')
            # Chromium's key dispatcher supplies the fullwidth layout key.
            key_session = page.context.new_cdp_session(page)
            key_session.send('Input.dispatchKeyEvent', {'type':'keyDown','key':'２','code':'Digit2','text':'２'})
            key_session.send('Input.dispatchKeyEvent', {'type':'keyUp','key':'２','code':'Digit2'})
            key_session.detach()
            page.keyboard.type('3456')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_otp_completions()===n+1', arg=before)
            index = semantic_index('Verification code', 6)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', index) == 6
            assert '6 of 6 digits' in page.locator('#accessible [role=status]').get_attribute('aria-label')
            assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
            page.keyboard.type('7')
            page.wait_for_timeout(40)
            assert page.evaluate('cuiGallery.exports.gallery_otp_completions()') == before + 1
            control_change('otp-mask', True)
            page.wait_for_timeout(40)
            texts = page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_command_text(i),cuiGallery.exports.gallery_command_length(i))))')
            assert texts.count('*') == 6 and not any(t in ['1','2','3','4','5','6'] for t in texts), texts
            click_semantic('Verification code', 6)
            page.keyboard.press('Backspace')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===5', arg=index)
            control_change('disabled', True)
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Verification code"]`)?.disabled===true')
            click_semantic('Verification code', 6)
            page.keyboard.type('9')
            page.wait_for_timeout(40)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', index) == 5
            assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
            control_change('disabled', False)
            click_semantic('Clear code')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===0', arg=index)
            page.evaluate('location.hash="Input Group"')
            page.wait_for_function('cuiGallery.renderedComponent==="Input Group"')
            assert page.evaluate('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>e.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==="Open address")}')
            group_input = page.locator('#accessible [role=textbox][aria-label="Workspace address"]')
            assert group_input.count() == 1
            action_count = page.evaluate('cuiGallery.exports.gallery_group_actions()')
            # Hold one host frame so pointer focus and all keyboard events route together.
            # No intermediate native focus wait: the prefix must proxy focus during routing.
            page.evaluate('(()=>{window.__cuiHeldFrames=[];window.__cuiOriginalRAF=requestAnimationFrame;window.requestAnimationFrame=cb=>{__cuiHeldFrames.push(cb);return 0;};})()')
            try:
                click_semantic('https://')
                page.keyboard.press('Control+a')
                page.keyboard.type('batch')
                assert page.evaluate('__cuiHeldFrames.length') > 0
            finally:
                page.evaluate('window.requestAnimationFrame=__cuiOriginalRAF;for(const cb of __cuiHeldFrames)requestAnimationFrame(cb);delete window.__cuiHeldFrames;delete window.__cuiOriginalRAF;')
            page.wait_for_function('document.querySelector("#accessible [role=status]")?.getAttribute("aria-label")?.includes("https://batch.example.com")')
            click_semantic('https://')
            # Passive addon focus is applied by the C reducer on its next frame.
            # Await that focus before sending keyboard events, including on slow CI hosts.
            group_index = semantic_index('Workspace address', 6)
            page.wait_for_function('(i)=>document.activeElement===document.querySelector("canvas") && cuiGallery.exports.gallery_semantic(i,7)===1', arg=group_index)
            page.keyboard.press('Control+a')
            page.keyboard.type('studio')
            page.wait_for_function('document.querySelector("#accessible [role=status]")?.getAttribute("aria-label")?.includes("https://studio.example.com")')
            click_semantic('Open address')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_group_actions()===n+1', arg=action_count)
            control_change('group-mode', '1')
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Workspace address"]`)?.getAttribute("aria-readonly")==="true"')
            click_semantic('Workspace address', 6)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=group_index)
            page.keyboard.press('Control+a')
            page.keyboard.type('blocked')
            page.wait_for_timeout(40)
            assert 'https://studio.example.com' in page.locator('#accessible [role=status]').get_attribute('aria-label')
            # Read-only editors leave application actions available.
            click_semantic('Open address')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_group_actions()===n+2', arg=action_count)
            control_change('group-mode', '3')
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Open address"]`)?.disabled===true')
            assert page.locator('#accessible button[aria-label="Open address"]').get_attribute('aria-busy') == 'true'
            click_semantic('Open address')
            page.wait_for_timeout(40)
            assert page.evaluate('cuiGallery.exports.gallery_group_actions()') == action_count + 2
            control_change('group-mode', '2')
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Workspace address"]`)?.disabled===true')
            assert page.locator('#accessible button[aria-label="Open address"]').is_disabled()
            control_change('group-mode', '0')
            page.evaluate('location.hash="Form"')
            page.wait_for_function('cuiGallery.renderedComponent==="Form"')
            assert page.locator('#accessible [role=textbox]').count() == 2
            for name in ['Name','Work email']:
                field = page.locator(f'#accessible [role=textbox][aria-label="{name}"]')
                assert field.get_attribute('aria-required') == 'true'
                label_id = field.get_attribute('aria-labelledby')
                assert label_id and page.locator(f'#{label_id}').get_attribute('aria-label') == name
                assert field.get_attribute('aria-description')
            count = page.evaluate('cuiGallery.exports.gallery_form_submissions()')
            click_semantic('Save profile')
            page.wait_for_function('(count)=>cuiGallery.exports.gallery_form_submissions()===count+1', arg=count)
            assert 'Profile saved' in page.locator('#accessible [role=status]').get_attribute('aria-label')
            control_change('form-mode', '1')
            email = page.locator('#accessible [role=textbox][aria-label="Work email"]')
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Work email"]`)?.getAttribute("aria-invalid")==="true"')
            assert 'containing @' in email.get_attribute('aria-description')
            click_semantic('Save profile')
            page.wait_for_function('document.querySelector("#accessible [role=status]")?.getAttribute("aria-label")==="Correct the highlighted fields."')
            assert page.evaluate('cuiGallery.exports.gallery_form_submissions()') == count + 1
            click_semantic('Work email', 6)
            page.keyboard.press('Control+a')
            page.keyboard.type('grace@example.com')
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Work email"]`)?.getAttribute("aria-invalid")==="false"')
            click_semantic('Save profile')
            page.wait_for_function('(count)=>cuiGallery.exports.gallery_form_submissions()===count+2', arg=count)
            control_change('form-mode', '2')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=textbox]")).every(e=>e.getAttribute("aria-readonly")==="true")')
            email_index = semantic_index('Work email', 6)
            email_length = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', email_index)
            click_semantic('Work email', 6)
            page.keyboard.press('Control+a')
            page.keyboard.type('invalid')
            page.wait_for_timeout(40)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', email_index) == email_length
            assert email.get_attribute('aria-invalid') == 'false'
            control_change('form-mode', '0')
            click_semantic('Save profile')
            page.wait_for_function('(count)=>cuiGallery.exports.gallery_form_submissions()===count+3', arg=count)
            control_change('form-mode', '3')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=textbox]")).every(e=>e.disabled)')
            assert page.locator('#accessible button[aria-label="Save profile"]').is_disabled()
            page.evaluate('location.hash="Dialog"')
            page.wait_for_function('cuiGallery.renderedComponent==="Dialog"')
            click_semantic('Edit preferences')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===1')
            assert page.locator('#accessible [role=dialog][aria-modal=true]').count() == 1
            assert page.locator('#accessible [role=dialog]').get_attribute('aria-labelledby')
            dialog_bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', semantic_index('Notification preferences'))
            footer_bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', semantic_index('Save changes'))
            assert footer_bounds[1] + footer_bounds[3] <= dialog_bounds[1] + dialog_bounds[3]
            body = semantic_index('Surface content')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,21)>0', arg=body)
            body_bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', body)
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+body_bounds[0]+body_bounds[2]/2,rectangle['y']+body_bounds[1]+body_bounds[3]/2)
            page.mouse.wheel(0,160)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,19)>0', arg=body)
            assert page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', semantic_index('Save changes')) == footer_bounds
            page.wait_for_function('cuiGallery.exports.gallery_redraw()===0')
            frame = page.evaluate('cuiGallery.frame')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.frame') == frame
            click_semantic('Save changes')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===0')
            index = semantic_index('Edit preferences')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', index) == 1
            click_semantic('Edit preferences')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===1')
            body = semantic_index('Surface content')
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+body_bounds[0]+body_bounds[2]/2,rectangle['y']+body_bounds[1]+body_bounds[3]/2)
            page.mouse.wheel(0,-1000)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,19)===0', arg=body)
            click_semantic('Advanced preferences')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(4)===1')
            assert page.locator('#accessible [role=dialog][aria-modal=true]').count() == 2
            for _ in range(5):
                page.keyboard.press('Tab')
                page.wait_for_timeout(20)
                assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).filter(i=>cuiGallery.exports.gallery_semantic(i,7)).every(i=>{const e=cuiGallery.exports;const name=new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)));return ["Close","Preview releases"].includes(name);})')
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(4)===0')
            assert page.evaluate('cuiGallery.exports.gallery_surface_open(0)') == 1
            index = semantic_index('Advanced preferences')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', index) == 1
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===0')
            page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')
            frame = page.evaluate('cuiGallery.frame')
            page.evaluate('(()=>{window.__cuiHeldFrames=[];window.__cuiOriginalRAF=requestAnimationFrame;window.requestAnimationFrame=cb=>{__cuiHeldFrames.push(cb);return 0;};})()')
            try:
                click_semantic('Edit preferences')
                rectangle = canvas.bounding_box()
                page.mouse.click(rectangle['x']+4, rectangle['y']+rectangle['height']-4)
                assert page.evaluate('__cuiHeldFrames.length') > 0
                assert page.evaluate('cuiGallery.exports.gallery_surface_open(0)') == 0
                assert page.evaluate('cuiGallery.frame') == frame
            finally:
                page.evaluate('(()=>{window.requestAnimationFrame=__cuiOriginalRAF;for(const cb of __cuiHeldFrames)requestAnimationFrame(cb);delete window.__cuiOriginalRAF;delete window.__cuiHeldFrames;})()')
            # Both clicks targeted the old layout. Dismiss the modal only after it is rendered.
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===1&&document.querySelector(`#accessible [role=dialog][aria-modal=true]`)')
            modal = semantic_index('Notification preferences', 17)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,3)>0&&cuiGallery.exports.gallery_semantic(i,4)>0', arg=modal)
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 1
            rectangle = canvas.bounding_box()
            page.mouse.click(rectangle['x']+4, rectangle['y']+rectangle['height']-4)
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===0')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('Edit preferences', 1))
            page.evaluate('location.hash="AlertDialog"')
            page.wait_for_function('cuiGallery.renderedComponent==="AlertDialog"')
            click_semantic('Delete workspace')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(1)===1')
            assert page.locator('#accessible [role=alertdialog][aria-modal=true]').count() == 1
            click_semantic('Delete')
            page.wait_for_function('cuiGallery.exports.gallery_alert_attempts()===1')
            assert page.evaluate('cuiGallery.exports.gallery_surface_open(1)') == 1
            page.wait_for_function('document.querySelector("#accessible [role=alertdialog]")?.getAttribute("aria-description")?.includes("rejected once")')
            click_semantic('Delete')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(1)===0')
            index = semantic_index('Delete workspace')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', index) == 1
            click_semantic('Delete workspace')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(1)===1')
            click_semantic('Keep workspace')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(1)===0')
            assert page.evaluate('cuiGallery.exports.gallery_alert_attempts()') == 0
            page.evaluate('location.hash="Sheet"')
            page.wait_for_function('cuiGallery.renderedComponent==="Sheet"')
            for edge in range(4):
                control_change('surface-variant', str(edge))
                click_semantic('Open settings sheet')
                page.wait_for_function('cuiGallery.exports.gallery_surface_open(2)===1')
                dialog = page.locator('#accessible [role=dialog]')
                assert dialog.get_attribute('aria-modal') == 'true'
                index = semantic_index('Workspace settings')
                bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', index)
                canvas_width = page.evaluate('document.querySelector("canvas").clientWidth')
                canvas_height = page.evaluate('document.querySelector("canvas").clientHeight')
                if edge == 0:
                    assert abs(bounds[0]) < 1
                elif edge == 1:
                    assert abs(bounds[0] + bounds[2] - canvas_width) < 1
                elif edge == 2:
                    assert abs(bounds[1]) < 1 and abs(bounds[2] - canvas_width) < 1
                else:
                    assert abs(bounds[1] + bounds[3] - canvas_height) < 1
                handle_name = 'Sheet width' if edge < 2 else 'Sheet height'
                handle = semantic_index(handle_name)
                assert page.locator('#accessible [role=slider]').get_attribute('aria-orientation') == ('horizontal' if edge < 2 else 'vertical')
                click_semantic(handle_name)
                page.keyboard.press('End')
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===cuiGallery.exports.gallery_semantic(i,13)', arg=handle)
                page.keyboard.press('Home')
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===100', arg=handle)
                handle_bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', handle)
                rectangle = canvas.bounding_box()
                x = rectangle['x'] + handle_bounds[0] + handle_bounds[2]/2
                y = rectangle['y'] + handle_bounds[1] + handle_bounds[3]/2
                delta = 26 if edge in (0,2) else -26
                page.mouse.move(x,y)
                page.mouse.down()
                page.mouse.move(x + (delta if edge < 2 else 0),y + (delta if edge >= 2 else 0),steps=5)
                page.mouse.up()
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)>120', arg=handle)
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_surface_open(2)===0')
                index = semantic_index('Open settings sheet')
                assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', index) == 1
            page.evaluate('location.hash="Popover"')
            page.wait_for_function('cuiGallery.renderedComponent==="Popover"')
            click_semantic('Open account card')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(3)===1')
            assert page.locator('#accessible [role=dialog][aria-modal=false]').count() == 1
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            click_semantic('Notify me about releases')
            page.wait_for_function('document.querySelector("#accessible [role=checkbox]")?.getAttribute("aria-checked")==="true"')
            rectangle = canvas.bounding_box()
            page.mouse.click(rectangle['x']+rectangle['width']-4, rectangle['y']+rectangle['height']-4)
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(3)===0')
            click_semantic('Open account card')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(3)===1')
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(3)===0')
            page.evaluate('location.hash="DropdownButton"')
            page.wait_for_function('cuiGallery.renderedComponent==="DropdownButton"')
            assert page.locator('#dropdown-variant').input_value() == '0'
            assert page.locator('#dropdown-variant option:checked').inner_text() == 'Default'
            default_colors = page.evaluate('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===0).map(i=>[5,6,7,8].map(f=>e.gallery_command(i,f)).join()).join("|")}')
            dropdown_select('variant', '1')
            assert page.locator('#dropdown-variant option:checked').inner_text() == 'Primary'
            primary_colors = page.evaluate('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===0).map(i=>[5,6,7,8].map(f=>e.gallery_command(i,f)).join()).join("|")}')
            assert default_colors != primary_colors
            dropdown_select('variant', '0')
            count = page.evaluate('cuiGallery.exports.gallery_clicks()')
            click_semantic('Export workspace')
            page.wait_for_function('(count)=>cuiGallery.exports.gallery_clicks()===count+1', arg=count)
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            page.keyboard.press('Tab')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            click_semantic('Priority')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == count + 1
            index = semantic_index('Export options')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,7)', index) == 1
            control_change('disabled', True)
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible button")).filter(e=>["Export workspace","Export options"].includes(e.textContent)).every(e=>e.disabled)')
            click_semantic('Export workspace')
            click_semantic('Export options')
            page.wait_for_timeout(40)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == count + 1
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            control_change('disabled', False)
            for variant in map(str, range(12)):
                frame = page.evaluate('cuiGallery.frame')
                dropdown_select('variant', variant)
                role = 36 if variant == '9' else 1
                page.wait_for_function('({frame,role})=>cuiGallery.frame>frame&&Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===role&&new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_semantic_name(i),cuiGallery.exports.gallery_semantic_length(i)))==="Export options")', arg={'frame': frame, 'role': role})
                assert semantic_index('Export workspace', role) is not None
                click_semantic('Export options', role)
                page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            dropdown_select('variant', '9')
            frame = page.evaluate('cuiGallery.frame')
            page.wait_for_function('document.querySelector(`#accessible [role=link][aria-label="Export workspace"]`)')
            action_count = page.evaluate('cuiGallery.exports.gallery_clicks()')
            click_semantic('Export workspace', 36)
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_clicks()===n+1', arg=action_count)
            page.keyboard.press('Enter')
            page.wait_for_function('(n)=>cuiGallery.exports.gallery_clicks()===n+2', arg=action_count)
            frame = page.evaluate('cuiGallery.frame')
            dropdown_select('variant', '0')
            page.wait_for_function('(frame)=>cuiGallery.frame>frame&&Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===1&&new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_semantic_name(i),cuiGallery.exports.gallery_semantic_length(i)))==="Export options")', arg=frame)
            for anchor in map(str, range(4)):
                frame = page.evaluate('cuiGallery.frame')
                dropdown_select('anchor', anchor)
                page.wait_for_function('(frame)=>cuiGallery.frame>frame&&Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===1&&new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_semantic_name(i),cuiGallery.exports.gallery_semantic_length(i)))==="Export options")', arg=frame)
                click_semantic('Export options', 1)
                page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
                menu = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', semantic_index('Export options', 15))
                assert menu[0] >= 0 and menu[1] >= 0 and menu[0]+menu[2] <= canvas.bounding_box()['width']+.2 and menu[1]+menu[3] <= canvas.bounding_box()['height']+.2
                trigger = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', semantic_index('Export options', 1))
                expected_x = trigger[0]+trigger[2]-menu[2] if anchor in ['0','2'] else trigger[0]
                expected_x = max(0,min(canvas.bounding_box()['width']-menu[2],expected_x))
                assert abs(menu[0]-expected_x) < .2
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            heights = []
            for size in ['3','1','0','2']:
                frame = page.evaluate('cuiGallery.frame')
                dropdown_select('size', size)
                page.wait_for_function('(f)=>cuiGallery.frame>f', arg=frame)
                heights.append(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Export workspace', 1)))
            assert heights == sorted(heights) and heights[0] < heights[-1], heights
            dropdown_select('size', '0')
            def dropdown_label_alpha():
                return page.evaluate('()=>{const e=cuiGallery.exports,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==="Export workspace");return i===undefined?-1:e.gallery_command(i,8)}')
            normal_alpha = dropdown_label_alpha()
            assert normal_alpha > 0
            for state in ['1','2']:
                dropdown_select('action', state)
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,5)===1', arg=semantic_index('Export workspace', 1))
                if state == '2':
                    assert abs(dropdown_label_alpha()-normal_alpha*.8) <= 1
                action_count = page.evaluate('cuiGallery.exports.gallery_clicks()')
                click_semantic('Export workspace', 1)
                click_semantic('Export options', 1)
                page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
                assert page.evaluate('cuiGallery.exports.gallery_clicks()') == action_count
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            dropdown_select('action', '0')
            for halves, absent, present in [('1','Export workspace','Export options'),('2','Export options','Export workspace')]:
                dropdown_select('halves', halves)
                page.wait_for_function('(name)=>!Array.from(document.querySelectorAll("#accessible [aria-label]")).some(e=>e.getAttribute("aria-label")===name)', arg=absent)
                click_semantic(present, 1)
                if halves == '1':
                    page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
                    page.keyboard.press('Escape')
                    page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
                else:
                    page.wait_for_function('(n)=>cuiGallery.exports.gallery_clicks()>n', arg=action_count)
            dropdown_select('halves', '0')
            dropdown_select('variant', '11')
            page.mouse.move(0,0)
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,5)===0', arg=semantic_index('Export workspace', 1))
            def dropdown_background():
                return page.evaluate('''(i)=>{const e=cuiGallery.exports,b=[1,2,3,4].map(f=>e.gallery_semantic(i,f));return Array.from({length:e.gallery_count()},(_,n)=>n).filter(n=>e.gallery_command(n,0)===0&&[1,2,3,4].every((f,k)=>Math.abs(e.gallery_command(n,f)-b[k])<.2)).map(n=>[5,6,7,8,12,13,14,15,16].map(f=>e.gallery_command(n,f)));}''', semantic_index('Export workspace', 1))
            page.wait_for_function('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>[5,6,7,8].map(f=>e.gallery_command(i,f)).join()).includes("65,40,160,51");}')
            background = dropdown_background()
            assert background and background[0][:4] == [65,40,160,51]
            assert background[0][4] == 1 and background[0][5] > 0 and background[0][6] == 0 and background[0][7] > 0 and background[0][8] == 0
            bounds = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', semantic_index('Export workspace', 1))
            rect = canvas.bounding_box()
            page.mouse.move(rect['x']+bounds[0]+bounds[2]/2,rect['y']+bounds[1]+bounds[3]/2)
            page.mouse.down()
            page.wait_for_function('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>[5,6,7,8].map(f=>e.gallery_command(i,f)).join()).includes("40,20,120,255");}')
            page.mouse.up()
            dropdown_check('outline')
            dropdown_check('selected')
            page.mouse.move(0,0)
            page.wait_for_function('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>[5,6,7,8].map(f=>e.gallery_command(i,f)).join()).includes("40,20,120,255");}')
            page.wait_for_function('()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>e.gallery_command(i,0)===1&&[5,6,7,8].map(f=>e.gallery_command(i,f)).join()==="65,40,160,102");}')
            page.evaluate('location.hash="Menu"')
            page.wait_for_function('cuiGallery.renderedComponent==="Menu"')
            click_semantic('Actions')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            page.keyboard.press('Home')
            page.keyboard.press('ArrowDown')
            page.keyboard.press('ArrowDown')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=menuitem]")).some(e=>e.textContent==="JSON")')
            assert page.locator('#accessible [role=menu]').count() == 2
            page.keyboard.press('ArrowLeft')
            page.wait_for_function('document.querySelectorAll("#accessible [role=menu]").length===1')
            click_semantic('Show hidden')
            page.wait_for_function('document.querySelector("#accessible [role=menuitemcheckbox]")?.getAttribute("aria-checked")==="true"')
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 1
            page.keyboard.press('End')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=menuitemradio]")).some(e=>e.textContent==="Compact")')
            click_semantic('Compact')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=menuitemradio]")).some(e=>e.textContent==="Compact"&&e.getAttribute("aria-checked")==="true")')
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            control_change('menu-kind', '1')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=button]")).some(e=>e.getAttribute("aria-label")==="Right-click this area")')
            click_semantic('Right-click this area')
            page.wait_for_timeout(50)
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            index = semantic_index('Right-click this area')
            bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', index)
            rectangle = canvas.bounding_box()
            page.mouse.click(rectangle['x']+bounds[0]+50,rectangle['y']+bounds[1]+bounds[3]/2,button='right')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            click_semantic('Priority')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            page.mouse.click(rectangle['x']+rectangle['width']-5,rectangle['y']+rectangle['height']-5,button='right')
            page.wait_for_timeout(50)
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            page.evaluate('location.hash="Scrollable"')
            page.wait_for_function('cuiGallery.renderedComponent==="Scrollable"')
            assert page.locator('#accessible [role=scrollbar]').count() == 2
            canvas.scroll_into_view_if_needed()
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+100,rectangle['y']+100)
            page.mouse.wheel(0,160)
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===32&&cuiGallery.exports.gallery_semantic(i,8)>0)')
            click_semantic('Vertical scroll')
            page.keyboard.press('End')
            index = semantic_index('Vertical scroll')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===cuiGallery.exports.gallery_semantic(i,13)', arg=index)
            page.keyboard.press('Home')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===0', arg=index)
            page.evaluate('location.hash="DataTable"')
            page.wait_for_function('cuiGallery.renderedComponent==="DataTable"')
            assert page.locator('#accessible [role=grid]').get_attribute('aria-rowcount') == '1000'
            assert 0 < page.locator('#accessible [role=gridcell]').count() < 40
            click_semantic('File 0001')
            page.keyboard.press('ArrowDown')
            page.wait_for_function('document.querySelector("#accessible [role=gridcell][aria-selected=true]")?.textContent==="File 0002"')
            page.keyboard.press('End')
            page.wait_for_function('document.querySelector("#accessible [role=gridcell][aria-selected=true]")?.getAttribute("aria-colindex")==="3"')
            assert page.locator('#accessible [role=gridcell][aria-selected=true]').get_attribute('aria-rowindex') == '2'
            page.keyboard.press('Home')
            page.wait_for_function('document.querySelector("#accessible [role=gridcell][aria-selected=true]")?.textContent==="File 0002"')
            control_change('table-selection', '0')
            canvas.focus()
            page.keyboard.press('End')
            page.wait_for_function('document.querySelector("#accessible [role=row][aria-selected=true]")?.getAttribute("aria-rowindex")==="1000"')
            assert page.locator('#accessible [role=gridcell]').filter(has_text='File 1000').count() == 1
            page.keyboard.press('Home')
            page.wait_for_function('document.querySelector("#accessible [role=row][aria-selected=true]")?.getAttribute("aria-rowindex")==="1"')
            page.keyboard.press('Escape')
            page.wait_for_function('!document.querySelector("#accessible [aria-selected=true]")')
            control_change('table-selection', '2')
            click_semantic('Name')
            page.wait_for_function('document.querySelector("#accessible [role=columnheader]")?.getAttribute("aria-sort")==="ascending"')
            page.keyboard.press('ArrowRight')
            page.keyboard.press('Enter')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=columnheader]")).some(e=>e.getAttribute("aria-label")==="Status"&&e.getAttribute("aria-sort")==="ascending")')
            # Native column movement preserves the key while changing visible order.
            page.keyboard.press('Control+ArrowRight')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=columnheader]")).map(e=>e.getAttribute("aria-label")).join(",")==="Name,Size,Status"')
            page.keyboard.press('Control+ArrowLeft')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=columnheader]")).map(e=>e.getAttribute("aria-label")).join(",")==="Name,Status,Size"')
            index = semantic_index('Column width')
            width_before = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', index)
            bounds = page.evaluate('(i)=>Array.from({length:4},(_,f)=>cuiGallery.exports.gallery_semantic(i,f+1))', index)
            rectangle = canvas.bounding_box()
            x,y = rectangle['x']+bounds[0]+bounds[2]/2,rectangle['y']+bounds[1]+bounds[3]/2
            page.mouse.move(x,y)
            page.mouse.down()
            page.mouse.move(x+24,y,steps=6)
            page.mouse.up()
            page.wait_for_function('(before)=>Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,0)===5&&cuiGallery.exports.gallery_semantic(i,8)>before)', arg=width_before)
            control_change('selection-mode', '1')
            page.wait_for_function('document.querySelector("#accessible [role=grid]")?.getAttribute("aria-multiselectable")==="true"')
            click_semantic('File 0001')
            page.keyboard.press('Shift+ArrowDown')
            page.wait_for_function('document.querySelectorAll("#accessible [role=gridcell][aria-selected=true]").length===2')
            page.keyboard.down('Control')
            click_semantic('File 0004')
            page.keyboard.up('Control')
            page.wait_for_function('document.querySelectorAll("#accessible [role=gridcell][aria-selected=true]").length===3')
            control_change('selection-mode', '0')
            canvas.scroll_into_view_if_needed()
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+100,rectangle['y']+140)
            page.mouse.wheel(0,160)
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,17)&&cuiGallery.exports.gallery_semantic(i,19)>0)')
            control_change('table-mode', '1')
            page.wait_for_function('document.querySelectorAll("#accessible [role=rowheader]").length>0')
            assert page.locator('#accessible [role=rowgroup]').count() > 0
            assert page.locator('#accessible [role=scrollbar]').count() > 0
            table_texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{const e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(table_texts + '.some(s=>s==="File details")')
            page.wait_for_function(table_texts + '.some(s=>s==="Storage")')
            hooks = next(text for text in page.evaluate(table_texts) if text.startswith('Visible row hooks:'))
            assert 0 < int(hooks.split(':')[1].split('/')[0]) < 20, hooks
            click_semantic('Inspect')
            page.wait_for_function(table_texts + '.some(s=>s.includes("inspections: 1"))')
            click_semantic('File 0001')
            page.wait_for_function('document.querySelector("#table-selection").value==="0"')
            page.wait_for_function('document.querySelector("#accessible [role=rowheader][aria-selected=true]")?.getAttribute("aria-rowindex")==="1"')
            control_change('table-mode', '2')
            page.wait_for_function(table_texts + '.some(s=>s.includes("Loading file metadata"))')
            assert page.locator('#accessible [role=gridcell]').count() == 0
            control_change('table-mode', '3')
            page.wait_for_function('document.querySelector("#accessible [role=grid]")?.getAttribute("aria-rowcount")==="0"')
            page.wait_for_function(table_texts + '.some(s=>s==="No files in this table.")')
            assert page.locator('#accessible [role=gridcell]').count() == 0
            page.evaluate('location.hash="Table"')
            page.wait_for_function('cuiGallery.renderedComponent==="Table"')
            assert page.locator('#accessible [role=rowgroup]').count() == 2
            assert page.locator('#accessible [role=columnheader]').count() == 3
            assert page.locator('#accessible [role=cell]').count() == 9
            page.evaluate('location.hash="Tree"')
            page.wait_for_function('cuiGallery.renderedComponent==="Tree"')
            assert page.locator('#accessible [role=treeitem]').count() == 5
            click_semantic('src')
            page.keyboard.press('ArrowLeft')
            page.wait_for_function('document.querySelectorAll("#accessible [role=treeitem]").length===3')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('document.querySelectorAll("#accessible [role=treeitem]").length===5')
            page.keyboard.press('ArrowDown')
            page.wait_for_function('document.querySelector("#accessible [role=treeitem][aria-selected=true]")?.getAttribute("aria-label")==="core.c"')
            assert page.locator('#accessible [role=treeitem][aria-label="core.c"]').get_attribute('aria-level') == '3'
            assert page.locator('#accessible button button').count() == 0
            page.evaluate('location.hash="Calendar"')
            page.wait_for_function('document.querySelector("#name").textContent==="Calendar" && cuiGallery.renderedComponent==="Calendar"')
            page.wait_for_timeout(80)
            click_semantic('2026-10-13')
            page.wait_for_function('cuiGallery.exports.gallery_semantic_count()>30')
            page.keyboard.press('ArrowRight')
            page.keyboard.press('Enter')
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(texts + '.some(s=>s.includes("Selected: 2026-10-14"))')
            page.evaluate('location.hash="DatePicker"')
            page.wait_for_function('document.querySelector("#name").textContent==="DatePicker" && cuiGallery.renderedComponent==="DatePicker"')
            page.wait_for_timeout(80)
            click_semantic('Date')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            click_semantic('2026-10-20')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            page.wait_for_function(texts + '.some(s=>s.includes("2026-10-20"))')
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
            page.evaluate('location.hash="TimeField"')
            page.wait_for_function('document.querySelector("#name").textContent==="TimeField" && cuiGallery.renderedComponent==="TimeField"')
            page.wait_for_timeout(80)
            click_semantic('Hour')
            page.keyboard.press('ArrowUp')
            page.wait_for_function(texts + '.some(s=>s.includes("10:30:00"))')
            page.evaluate('location.hash="ColorPicker"')
            page.wait_for_function('document.querySelector("#name").textContent==="ColorPicker" && cuiGallery.renderedComponent==="ColorPicker"')
            page.wait_for_timeout(80)
            canvas.scroll_into_view_if_needed()
            canvas.focus()
            page.keyboard.press('Tab')
            index = semantic_index('Hue')
            hue = page.evaluate('(index)=>cuiGallery.exports.gallery_semantic(index,8)', index)
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(args)=>cuiGallery.exports.gallery_semantic(args.index,8)>args.hue', arg={'index':index,'hue':hue})
            page.evaluate('location.hash="Select"')
            page.wait_for_function('document.querySelector("#name").textContent==="Select" && cuiGallery.renderedComponent==="Select"')
            page.wait_for_timeout(80)
            control_change('selection-mode', '1')
            page.wait_for_timeout(60)
            click_semantic('Delivery')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            assert page.locator('#accessible [role=listbox][aria-multiselectable=true]').count() >= 1, page.locator('#accessible').inner_html()
            click_semantic('Standard')
            page.wait_for_timeout(60)
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 1
            assert page.evaluate('(index)=>cuiGallery.exports.gallery_semantic(index,11)', semantic_index('Standard')) == 1
            click_semantic('Priority')
            page.wait_for_timeout(60)
            assert page.evaluate('(index)=>cuiGallery.exports.gallery_semantic(index,11)', semantic_index('Priority')) == 1
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            page.evaluate('location.hash="Skeleton"')
            page.wait_for_function('document.querySelector("#name").textContent==="Skeleton" && cuiGallery.renderedComponent==="Skeleton"')
            signature = 'JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))'
            before = page.evaluate('''()=>({frame:cuiGallery.frame,signature:JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))})''')
            page.wait_for_function('''before=>cuiGallery.frame>before.frame&&JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))!==before.signature''', arg=before)
            before_reduce = page.evaluate('cuiGallery.frame')
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('(frame)=>cuiGallery.frame>frame&&cuiGallery.exports.gallery_redraw()===0', arg=before_reduce)
            settled = page.evaluate('''()=>({frame:cuiGallery.frame,signature:JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))})''')
            after = page.evaluate('''async()=>{for(let i=0;i<4;i++)await new Promise(resolve=>requestAnimationFrame(resolve));return {frame:cuiGallery.frame,signature:JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))};}''')
            assert after == settled, (settled, after)
            page.emulate_media(reduced_motion='no-preference')
            page.wait_for_function('cuiGallery.exports.gallery_redraw()===1')
            page.evaluate('location.hash="Clipboard"')
            page.wait_for_function('document.querySelector("#name").textContent==="Clipboard" && cuiGallery.renderedComponent==="Clipboard"')
            page.wait_for_timeout(80)
            box = canvas.bounding_box()
            for index, expected in [(0, 'Copied: Hello from CUI'), (1, 'Read: Hello from CUI')]:
                bounds = page.evaluate('(index)=>Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(index,i+1))', index)
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
                page.wait_for_function('(expected)=>' + texts + '.some(s=>s.includes(expected))', arg=expected)
            page.evaluate('navigator.clipboard.writeText("é".repeat(100))')
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(1,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function(texts + '.some(s=>s.includes("Read: é"))')
            assert page.evaluate(texts + '.every(s=>!s.includes("�"))')
            page.evaluate('location.hash="Focus%20Trap"')
            page.wait_for_function('document.querySelector("#name").textContent==="Focus Trap" && cuiGallery.renderedComponent==="Focus Trap"')
            page.wait_for_timeout(80)
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function('cuiGallery.exports.gallery_modal()===1')
            initial_focus = page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,7))')
            page.keyboard.press('Tab')
            page.keyboard.press('Tab')
            page.wait_for_timeout(60)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>cuiGallery.exports.gallery_semantic(i,7))') == initial_focus
            assert page.evaluate('document.activeElement.id') == 'preview'
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
            for name, initial, key, expected in [('NumberInput', 42, 'ArrowUp', 43), ('Rating', 3, 'ArrowRight', 3.5)]:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(80)
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,8)') == initial
                canvas.focus()
                page.keyboard.press('Tab')
                page.keyboard.press(key)
                page.wait_for_function('(expected)=>cuiGallery.exports.gallery_semantic(0,8)===expected', arg=expected)
            page.evaluate('location.hash="Combobox"')
            page.wait_for_function('document.querySelector("#name").textContent==="Combobox" && cuiGallery.renderedComponent==="Combobox"')
            page.wait_for_timeout(80)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.type('Ban')
            page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(texts + '.some(s=>s.includes("1 matching options"))')
            page.keyboard.press('ArrowDown')
            page.keyboard.press('Enter')
            page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
            for name in ['Select', 'Menu']:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(80)
                box = canvas.bounding_box()
                bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
                page.keyboard.press('Escape')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
                page.keyboard.press('Enter')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 1')
                page.keyboard.press('Tab')
                page.wait_for_timeout(50)
                assert page.evaluate('document.activeElement.id') == 'preview'
                page.keyboard.press('ArrowDown')
                page.keyboard.press('Enter')
                page.wait_for_function('cuiGallery.exports.gallery_modal() === 0')
                assert page.evaluate('cuiGallery.exports.gallery_semantic(0,7)') == 1
            page.evaluate('location.hash="Tabs"')
            page.wait_for_function('document.querySelector("#name").textContent==="Tabs" && cuiGallery.renderedComponent==="Tabs"')
            page.wait_for_timeout(80)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>{const e=cuiGallery.exports;return e.gallery_semantic(i,0)===8 && e.gallery_semantic(i,11) && new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))==="Activity";}).some(Boolean)')
            control_change('tabs-manual', True)
            click_semantic('Overview', 8)
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,7)===1', arg=semantic_index('Activity', 8))
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,11)', semantic_index('Overview', 8)) == 1
            page.keyboard.press('Enter')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,11)===1', arg=semantic_index('Activity', 8))
            control_change('tabs-manual', False)
            for variant in ['0','1','2','3','4']:
                control_change('tabs-variant', variant)
                heights = []
                for size in ['3','1','0','2']:
                    frame = page.evaluate('cuiGallery.frame')
                    control_change('tabs-size', size)
                    page.wait_for_function('(old)=>cuiGallery.frame>old', arg=frame)
                    heights.append(page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,4)', semantic_index('Overview', 8)))
                assert heights == sorted(heights) and heights[0] < heights[-1], (variant, heights)
            control_change('tabs-variant', '1')
            control_change('tabs-size', '0')
            for mode in ['1','2']:
                control_change('tabs-mode', mode)
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(1)>0')
                click_semantic('Reveal last tab', 1)
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,11)===1', arg=semantic_index('Audit history', 8))
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)>0')
                e = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', semantic_index('Audit history', 8))
                viewport = page.evaluate('[2,3,4,5].map(f=>cuiGallery.exports.gallery_tabs_scroll(f))')
                axis = 1 if mode == '2' else 0
                assert e[axis] >= viewport[axis]-.2 and e[axis]+e[axis+2] <= viewport[axis]+viewport[axis+2]+.2
                click_semantic('Tab scrollbar', 32)
                page.keyboard.press('Home')
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)===0')
                page.keyboard.press('End')
                page.wait_for_function('Math.abs(cuiGallery.exports.gallery_tabs_scroll(0)-cuiGallery.exports.gallery_tabs_scroll(1))<.2')
                click_semantic('All tabs', 1)
                page.wait_for_function('document.querySelector(`#accessible [role=menu][aria-label="All tabs"]`)')
                page.keyboard.press('Home')
                page.keyboard.press('Enter')
                page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,11)===1', arg=semantic_index('Overview', 8))
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)===0')
                viewport = page.evaluate('[2,3,4,5].map(f=>cuiGallery.exports.gallery_tabs_scroll(f))')
                rect = canvas.bounding_box()
                page.mouse.move(rect['x']+viewport[0]+viewport[2]/2,rect['y']+viewport[1]+viewport[3]/2)
                page.mouse.wheel(120 if mode=='1' else 0,120 if mode=='2' else 0)
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)>0')
                click_semantic('Tab scrollbar', 32)
                page.keyboard.press('Home')
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)===0')
                bar = page.evaluate('(i)=>[1,2,3,4].map(f=>cuiGallery.exports.gallery_semantic(i,f))', semantic_index('Tab scrollbar', 32))
                page.mouse.move(rect['x']+bar[0]+min(bar[2]/2,3),rect['y']+bar[1]+min(bar[3]/2,3))
                page.mouse.down()
                page.mouse.move(rect['x']+bar[0]+bar[2]-2,rect['y']+bar[1]+bar[3]-2,steps=4)
                page.mouse.up()
                page.wait_for_function('cuiGallery.exports.gallery_tabs_scroll(0)>0')
            control_change('tabs-mode', '0')
            for name, expected in [('Dock', '2 tab regions'), ('Plot', '16.67')]:
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(80)
                box = canvas.bounding_box()
                bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
                page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
                texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
                page.wait_for_function('(expected)=>' + texts + '.some(s=>s.includes(expected))', arg=expected)
                assert 'base model' in page.locator('#status').inner_text()
            page.evaluate('location.hash="Tooltip"')
            page.wait_for_function('document.querySelector("#name").textContent==="Tooltip" && cuiGallery.renderedComponent==="Tooltip"')
            page.wait_for_timeout(80)
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            box = canvas.bounding_box()
            page.mouse.move(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            texts = 'Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));})'
            page.wait_for_function(texts + '.some(s=>s.includes("floating tooltip"))')
            canvas.focus()
            page.keyboard.press('Escape')
            page.wait_for_function(texts + '.every(s=>!s.includes("floating tooltip"))')
            page.evaluate('location.hash="HoverCard"')
            page.wait_for_function('document.querySelector("#name").textContent==="HoverCard" && cuiGallery.renderedComponent==="HoverCard"')
            page.wait_for_timeout(80)
            page.mouse.move(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_function(texts + '.some(s=>s.includes("Alex Morgan"))')
            page.mouse.move(box['x'] + box['width'] - 8, box['y'] + box['height'] - 8)
            page.locator('#theme').focus()
            page.wait_for_function('cuiGallery.exports.gallery_hover_state(0)===0 && cuiGallery.exports.gallery_hover_state(1)===0')
            page.wait_for_function('cuiGallery.exports.gallery_hover_state(2)===0')
            page.wait_for_function(texts + '.every(s=>!s.includes("Alex Morgan"))')
            page.evaluate('location.hash="Checkbox"')
            page.wait_for_function('document.querySelector("#name").textContent==="Checkbox" && cuiGallery.renderedComponent==="Checkbox"')
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.press('Space')
            page.wait_for_function('cuiGallery.exports.gallery_semantic(0,6) === 1')
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>cuiGallery.exports.gallery_command(i,0)).includes(5)')
            assert page.locator('#accessible [role=checkbox]').get_attribute('aria-checked') == 'true'
            control_change('checkbox-state', '2')
            page.wait_for_function('document.querySelector(`#accessible [role=checkbox]`).getAttribute("aria-checked")==="mixed"')
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,8)') == 2
            page.keyboard.press('Space')
            page.wait_for_function('document.querySelector(`#accessible [role=checkbox]`).getAttribute("aria-checked")==="true"')
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).filter(i=>cuiGallery.exports.gallery_command(i,0)===2).every(i=>!/[x✓]/.test(new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_command_text(i),cuiGallery.exports.gallery_command_length(i)))))')
            glyph_widths = []
            for size in ['3','1','0','2']:
                control_change('control-size', size)
                assert page.locator('#accessible [role=checkbox]').count() == 1
                glyph_widths.append(page.evaluate('()=>{const e=cuiGallery.exports,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===5);return i===undefined?0:e.gallery_command(i,3)}'))
            assert glyph_widths == sorted(glyph_widths) and len(set(glyph_widths)) == 4, glyph_widths
            control_change('choice-required', True)
            control_change('choice-invalid', True)
            assert page.locator('#accessible [role=checkbox]').get_attribute('aria-required') == 'true'
            assert page.locator('#accessible [role=checkbox]').get_attribute('aria-invalid') == 'true'
            control_change('choice-skip-tab', True)
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,9)') == -1
            control_change('choice-skip-tab', False)
            control_change('disabled', True)
            before = page.evaluate('cuiGallery.exports.gallery_semantic(0,8)')
            click_semantic('Receive updates', 2)
            assert page.evaluate('cuiGallery.exports.gallery_semantic(0,8)') == before
            control_change('disabled', False)
            page.evaluate('location.hash="Radio"')
            page.wait_for_function('cuiGallery.renderedComponent==="Radio"')
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).filter(i=>cuiGallery.exports.gallery_command(i,0)===2).every(i=>!/[()*]/.test(new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_command_text(i),cuiGallery.exports.gallery_command_length(i)))))')
            click_semantic('Priority', 4)
            page.wait_for_function('document.querySelector(`#accessible [role=radio][aria-label=Priority]`).getAttribute("aria-checked")==="true"')
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>cuiGallery.exports.gallery_command(i,0)).includes(5)')
            assert page.locator('#accessible [role=radio][aria-label=Priority]').get_attribute('aria-required') == 'true'
            assert page.locator('#accessible [role=radio][aria-label=Priority]').get_attribute('aria-invalid') == 'true'
            control_change('choice-skip-tab', True)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>cuiGallery.exports.gallery_semantic(i,9)).every(n=>n===-1)')
            control_change('choice-skip-tab', False)
            control_change('choice-required', False)
            control_change('choice-invalid', False)
            page.evaluate('location.hash="Switch"')
            page.wait_for_function('cuiGallery.renderedComponent==="Switch"')
            assert page.locator('#accessible [role=switch]').get_attribute('aria-checked') == 'true'
            control_change('switch-color', True)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>{const e=cuiGallery.exports;return e.gallery_command(i,0)===0&&[5,6,7].every((f,j)=>e.gallery_command(i,f)===[180,74,44][j]);})')
            assert page.evaluate('()=>{const e=cuiGallery.exports,c=document.querySelector("canvas"),s=c.width/c.clientWidth,i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===0&&[5,6,7].every((f,j)=>e.gallery_command(i,f)===[180,74,44][j]));if(i===undefined)return false;const x=Math.floor((e.gallery_command(i,1)+5)*s),y=Math.floor((e.gallery_command(i,2)+e.gallery_command(i,4)/2)*s),p=c.getContext("2d").getImageData(x,y,1,1).data;return Math.abs(p[0]-180)<3&&Math.abs(p[1]-74)<3&&Math.abs(p[2]-44)<3;}')
            control_change('switch-side', True)
            assert page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).some(i=>{const e=cuiGallery.exports;return e.gallery_command(i,0)===2&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))==="Notifications"&&e.gallery_command(i,1)<e.gallery_semantic(0,1)+e.gallery_semantic(0,3)/2;})')
            click_semantic('Notifications', 3)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(1)===0')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(0)===0')
            assert page.locator('#accessible [role=switch]').get_attribute('aria-checked') == 'false'
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(2)===0')
            click_semantic('Notifications', 3)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(1)>0')
            assert page.evaluate('cuiGallery.exports.gallery_control_stats(0)') == page.evaluate('cuiGallery.exports.gallery_control_stats(1)')
            assert page.evaluate('cuiGallery.exports.gallery_control_stats(2)') == 0
            page.emulate_media(reduced_motion='no-preference')
            page.evaluate('location.hash="Toggle"')
            page.wait_for_function('cuiGallery.renderedComponent==="Toggle"')
            assert page.locator('#accessible [role=toolbar][aria-label="Formatting options"]').count() == 1
            assert page.locator('#accessible [role=toolbar][aria-label="Formatting options"]').get_attribute('aria-orientation') == 'horizontal'
            assert page.evaluate('()=>document.querySelector("canvas").style.cursor===["default","pointer","text","ew-resize","ns-resize","move","not-allowed"][cuiGallery.exports.gallery_cursor()]')
            assert page.locator('#accessible button[aria-label="Bold"]').get_attribute('aria-pressed') == 'true'
            click_semantic('Pin this item', 1)
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Pin this item"]`).getAttribute("aria-pressed")==="true"')
            click_semantic('Italic', 1)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(4)===0&&cuiGallery.exports.gallery_control_stats(5)===1')
            control_change('toggle-selection', '1')
            click_semantic('Underline', 1)
            page.wait_for_function('cuiGallery.exports.gallery_control_stats(5)===1&&cuiGallery.exports.gallery_control_stats(6)===1')
            control_change('toggle-segmented', True)
            control_change('toggle-variant', '1')
            assert page.locator('#accessible button[aria-label="Underline"]').get_attribute('aria-pressed') == 'true'
            control_change('disabled', True)
            click_semantic('Bold', 1)
            assert page.evaluate('cuiGallery.exports.gallery_control_stats(4)') == 0
            control_change('disabled', False)
            control_change('toggle-selection', '0')
            assert page.evaluate('cuiGallery.exports.gallery_control_stats(5)') == 1
            assert page.evaluate('cuiGallery.exports.gallery_control_stats(6)') == 0
            page.evaluate('location.hash="Slider"')
            page.wait_for_function('document.querySelector("#name").textContent==="Slider" && cuiGallery.renderedComponent==="Slider"')
            page.wait_for_timeout(60)
            canvas.focus()
            page.keyboard.press('Tab')
            initial = page.evaluate('cuiGallery.exports.gallery_semantic(0,8)')
            page.keyboard.press('ArrowRight')
            page.wait_for_function('(initial)=>cuiGallery.exports.gallery_semantic(0,8)>initial', arg=initial)
            page.evaluate('location.hash="Input"')
            page.wait_for_function('document.querySelector("#name").textContent==="Input" && cuiGallery.renderedComponent==="Input"')
            page.wait_for_timeout(60)
            canvas.focus()
            page.keyboard.press('Tab')
            page.keyboard.type('Z')
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>{let e=cuiGallery.exports;return new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)));}).some(s=>s.includes("Z"))')
            page.keyboard.press('Control+a')
            page.keyboard.type('Selection test')
            page.wait_for_function('cuiGallery.exports.gallery_input_selection(1)===14')
            page.keyboard.press('Control+z')
            page.wait_for_function('cuiGallery.exports.gallery_input_selection(1)===13')
            page.keyboard.press('Control+y')
            page.wait_for_function('cuiGallery.exports.gallery_input_selection(1)===14')
            bounds = page.evaluate('Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(0,i+1))')
            rectangle = canvas.bounding_box()
            y = rectangle['y'] + bounds[1] + bounds[3] - 16
            page.mouse.move(rectangle['x']+bounds[0]+12, y)
            page.mouse.down()
            page.mouse.move(rectangle['x']+bounds[0]+100, y, steps=8)
            page.mouse.up()
            page.wait_for_function('cuiGallery.exports.gallery_input_selection(0)!==cuiGallery.exports.gallery_input_selection(1)')
            page.keyboard.type('Q')
            page.wait_for_function('cuiGallery.exports.gallery_input_selection(0)===cuiGallery.exports.gallery_input_selection(1)')
            control_change('input-mode', '1')
            page.wait_for_function('document.querySelector("#accessible [role=textbox]")?.getAttribute("aria-readonly")==="true"')
            length = page.evaluate('cuiGallery.exports.gallery_input_selection(2)')
            canvas.focus()
            page.keyboard.type('R')
            page.wait_for_timeout(50)
            assert page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == length
            control_change('input-mode', '2')
            page.wait_for_function('document.querySelector("#accessible [role=textbox]")?.getAttribute("aria-invalid")==="true"')
            assert page.locator('#accessible [role=textbox]').get_attribute('aria-required') == 'true'
            assert page.locator('#accessible [role=textbox]').get_attribute('aria-readonly') == 'false'
            page.evaluate('location.hash="Chart"')
            page.wait_for_function('document.querySelector("#name").textContent==="Chart" && cuiGallery.renderedComponent==="Chart"')
            assert page.locator('#pending').is_visible()
            assert not page.locator('#preview-card').is_visible()
            page.locator('#search').fill('button')
            assert 0 < page.locator('#catalog a').count() < 78
            page.locator('#search').fill('')
            page.set_viewport_size({'width': 390, 'height': 844})
            page.locator('#menu').click()
            assert page.locator('#sidebar').is_visible()
            page.locator('#catalog a', has_text='Button').first.click()
            page.wait_for_function('document.querySelector("#name").textContent==="Button" && cuiGallery.renderedComponent==="Button"')
            page.wait_for_timeout(100)
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
            overflow = []
            for name in page.evaluate('Object.keys(cuiGallery.stories)'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(40)
                bad = page.evaluate("""()=>{
                    const e=cuiGallery.exports,canvas=document.querySelector('canvas'),clips=[],bad=[];
                    const intersect=(a,b)=>[Math.max(a[0],b[0]),Math.max(a[1],b[1]),Math.min(a[2],b[2]),Math.min(a[3],b[3])];
                    for(let i=0;i<e.gallery_count();i++){
                        const type=e.gallery_command(i,0),x=e.gallery_command(i,1),y=e.gallery_command(i,2),w=e.gallery_command(i,3),h=e.gallery_command(i,4),rect=[x,y,x+w,y+h];
                        if(type===3){clips.push(clips.length?intersect(rect,clips.at(-1)):rect);continue;}
                        if(type===4){clips.pop();continue;}
                        if(type!==2)continue;
                        const visible=clips.length?intersect(rect,clips.at(-1)):rect;
                        if(visible[2]<=visible[0]||visible[3]<=visible[1])continue;
                        if(visible[0]<-1||visible[1]<-1||visible[2]>canvas.clientWidth+1||visible[3]>canvas.clientHeight+1)bad.push(visible);
                    }
                    return bad;
                }""")
                if bad:
                    overflow.append((name, bad))
                assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'), name
            assert not overflow, overflow
            page.evaluate('location.hash="DataTable"')
            page.wait_for_function('cuiGallery.renderedComponent==="DataTable"')
            control_change('table-pin', '1')
            page.wait_for_timeout(50)
            index = semantic_index('Name')
            fixed_x = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', index)
            canvas.scroll_into_view_if_needed()
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+200,rectangle['y']+120)
            page.mouse.wheel(120,0)
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,17)&&cuiGallery.exports.gallery_semantic(i,18)>0)')
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,1)', semantic_index('Name')) == fixed_x
            page.evaluate('location.hash="Button"')
            page.wait_for_timeout(100)
            page.wait_for_function('document.querySelector("#name").textContent==="Button" && cuiGallery.renderedComponent==="Button"')
            page.wait_for_timeout(50)
            page.screenshot(path=str(root / 'browser-mobile.png'))
            page.evaluate('location.hash="%"')
            page.wait_for_timeout(60)
            assert page.locator('#name').inner_text() == 'Button'
            assert not errors, errors
            browser.close()
            print('PASS: 78 catalog entries, 70 real WASM stories, Sidebar/Textarea interactions, controlled form validation, surface focus/dismissal, split-button actions, table interactions, themes, pending status, search, 390px layout, /cui/ hosting')
    finally:
        server.shutdown()
        server.server_close()

if __name__ == '__main__':
    main()
