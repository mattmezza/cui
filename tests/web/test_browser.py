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
            page.wait_for_function('window.cuiGallery && window.cuiGallery.exports.gallery_count() > 0')
            assert page.request.get(f'http://127.0.0.1:{server.server_port}/cui/LICENSE.txt').ok
            assert page.locator('#catalog a').count() == 78
            assert page.locator('#name').inner_text() == 'Button'
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
            page.locator('#disabled').check()
            page.wait_for_function('cuiGallery.exports.gallery_semantic(0,5) === 1')
            page.mouse.click(box['x'] + bounds[0] + bounds[2]/2, box['y'] + bounds[1] + bounds[3]/2)
            page.wait_for_timeout(80)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == 2
            height = page.evaluate('cuiGallery.exports.gallery_semantic(0,4)')
            page.locator('#density').select_option('1')
            page.wait_for_function('(height)=>cuiGallery.exports.gallery_semantic(0,4)<height', arg=height)
            page.locator('#density').select_option('0')
            assert page.locator('#features tr').count() > 0
            assert page.locator('#reference').get_attribute('href').startswith('https://gpui-kit.com/')
            page.locator('#theme').select_option('1')
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
            assert pixels[:5] == [blue,blue,orange,orange,blue], pixels
            assert pixels[5] not in [blue,orange], pixels
            for name in page.evaluate('Object.keys(cuiGallery.stories)'):
                page.evaluate('(name)=>location.hash=name', name)
                page.wait_for_function('(name)=>document.querySelector("#name").textContent===name && cuiGallery.renderedComponent===name', arg=name)
                page.wait_for_timeout(50)
                assert page.locator('#preview-card').is_visible()
                assert 'draw commands' in page.locator('#preview-note').inner_text()
            def semantic_index(name, role=None):
                return page.evaluate('({name,role})=>Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).find(i=>{const e=cuiGallery.exports;return (role===null?e.gallery_semantic(i,0)!==27:e.gallery_semantic(i,0)===role)&&new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)))===name;})', {'name':name,'role':role})
            def click_semantic(name, role=None):
                index = semantic_index(name, role)
                assert index is not None, name
                canvas.scroll_into_view_if_needed()
                bounds = page.evaluate('(index)=>Array.from({length:4},(_,i)=>cuiGallery.exports.gallery_semantic(index,i+1))', index)
                rectangle = canvas.bounding_box()
                page.mouse.click(rectangle['x']+bounds[0]+bounds[2]/2,rectangle['y']+bounds[1]+bounds[3]/2)
            page.evaluate('location.hash="OtpInput"')
            page.wait_for_function('cuiGallery.renderedComponent==="OtpInput"')
            otp = page.locator('#accessible [role=textbox][aria-label="Verification code"]')
            assert otp.count() == 1
            def otp_cells():
                return page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>i).filter(i=>{const e=cuiGallery.exports;return e.gallery_command(i,0)===0&&e.gallery_command(i,3)===32&&e.gallery_command(i,4)===32;}).map(i=>cuiGallery.exports.gallery_command(i,1))')
            cells = otp_cells()
            assert len(cells) == 6, cells
            assert [round(cells[i+1]-cells[i]) for i in range(5)] == [36,36,52,36,36]
            page.locator('#otp-groups').select_option('3')
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
            page.locator('#otp-mask').check()
            page.wait_for_timeout(40)
            texts = page.evaluate('Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>new TextDecoder().decode(new Uint8Array(cuiGallery.exports.memory.buffer,cuiGallery.exports.gallery_command_text(i),cuiGallery.exports.gallery_command_length(i))))')
            assert texts.count('*') == 6 and not any(t in ['1','2','3','4','5','6'] for t in texts), texts
            click_semantic('Verification code', 6)
            page.keyboard.press('Backspace')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===5', arg=index)
            page.locator('#disabled').check()
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Verification code"]`)?.disabled===true')
            click_semantic('Verification code', 6)
            page.keyboard.type('9')
            page.wait_for_timeout(40)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', index) == 5
            assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
            page.locator('#disabled').uncheck()
            click_semantic('Clear code')
            page.wait_for_function('(i)=>cuiGallery.exports.gallery_semantic(i,8)===0', arg=index)
            page.evaluate('location.hash="Input Group"')
            page.wait_for_function('cuiGallery.renderedComponent==="Input Group"')
            group_input = page.locator('#accessible [role=textbox][aria-label="Workspace address"]')
            assert group_input.count() == 1
            action_count = page.evaluate('cuiGallery.exports.gallery_group_actions()')
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
            page.locator('#group-mode').select_option('1')
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
            page.locator('#group-mode').select_option('3')
            page.wait_for_function('document.querySelector(`#accessible button[aria-label="Open address"]`)?.disabled===true')
            assert page.locator('#accessible button[aria-label="Open address"]').get_attribute('aria-busy') == 'true'
            click_semantic('Open address')
            page.wait_for_timeout(40)
            assert page.evaluate('cuiGallery.exports.gallery_group_actions()') == action_count + 2
            page.locator('#group-mode').select_option('2')
            page.wait_for_function('document.querySelector(`#accessible [role=textbox][aria-label="Workspace address"]`)?.disabled===true')
            assert page.locator('#accessible button[aria-label="Open address"]').is_disabled()
            page.locator('#group-mode').select_option('0')
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
            page.locator('#form-mode').select_option('1')
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
            page.locator('#form-mode').select_option('2')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=textbox]")).every(e=>e.getAttribute("aria-readonly")==="true")')
            email_index = semantic_index('Work email', 6)
            email_length = page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', email_index)
            click_semantic('Work email', 6)
            page.keyboard.press('Control+a')
            page.keyboard.type('invalid')
            page.wait_for_timeout(40)
            assert page.evaluate('(i)=>cuiGallery.exports.gallery_semantic(i,8)', email_index) == email_length
            assert email.get_attribute('aria-invalid') == 'false'
            page.locator('#form-mode').select_option('0')
            click_semantic('Save profile')
            page.wait_for_function('(count)=>cuiGallery.exports.gallery_form_submissions()===count+3', arg=count)
            page.locator('#form-mode').select_option('3')
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
            click_semantic('Edit preferences')
            rectangle = canvas.bounding_box()
            page.mouse.click(rectangle['x']+4, rectangle['y']+rectangle['height']-4)
            page.wait_for_function('cuiGallery.exports.gallery_surface_open(0)===0')
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
                page.locator('#surface-variant').select_option(str(edge))
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
            page.locator('#disabled').check()
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible button")).filter(e=>["Export workspace","Export options"].includes(e.textContent)).every(e=>e.disabled)')
            click_semantic('Export workspace')
            click_semantic('Export options')
            page.wait_for_timeout(40)
            assert page.evaluate('cuiGallery.exports.gallery_clicks()') == count + 1
            assert page.evaluate('cuiGallery.exports.gallery_modal()') == 0
            page.locator('#disabled').uncheck()
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
            click_semantic('Compact')
            page.wait_for_function('Array.from(document.querySelectorAll("#accessible [role=menuitemradio]")).some(e=>e.textContent==="Compact"&&e.getAttribute("aria-checked")==="true")')
            page.keyboard.press('Escape')
            page.wait_for_function('cuiGallery.exports.gallery_modal()===0')
            page.locator('#menu-kind').select_option('1')
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
            page.locator('#table-selection').select_option('0')
            canvas.focus()
            page.keyboard.press('End')
            page.wait_for_function('document.querySelector("#accessible [role=row][aria-selected=true]")?.getAttribute("aria-rowindex")==="1000"')
            assert page.locator('#accessible [role=gridcell]').filter(has_text='File 1000').count() == 1
            page.keyboard.press('Home')
            page.wait_for_function('document.querySelector("#accessible [role=row][aria-selected=true]")?.getAttribute("aria-rowindex")==="1"')
            page.keyboard.press('Escape')
            page.wait_for_function('!document.querySelector("#accessible [aria-selected=true]")')
            page.locator('#table-selection').select_option('2')
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
            page.locator('#selection-mode').select_option('1')
            page.wait_for_function('document.querySelector("#accessible [role=grid]")?.getAttribute("aria-multiselectable")==="true"')
            click_semantic('File 0001')
            page.keyboard.press('Shift+ArrowDown')
            page.wait_for_function('document.querySelectorAll("#accessible [role=gridcell][aria-selected=true]").length===2')
            page.keyboard.down('Control')
            click_semantic('File 0004')
            page.keyboard.up('Control')
            page.wait_for_function('document.querySelectorAll("#accessible [role=gridcell][aria-selected=true]").length===3')
            page.locator('#selection-mode').select_option('0')
            canvas.scroll_into_view_if_needed()
            rectangle = canvas.bounding_box()
            page.mouse.move(rectangle['x']+100,rectangle['y']+140)
            page.mouse.wheel(0,160)
            page.wait_for_function('Array.from({length:cuiGallery.exports.gallery_semantic_count()},(_,i)=>i).some(i=>cuiGallery.exports.gallery_semantic(i,17)&&cuiGallery.exports.gallery_semantic(i,19)>0)')
            page.locator('#table-mode').select_option('1')
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
            page.locator('#table-mode').select_option('2')
            page.wait_for_function(table_texts + '.some(s=>s.includes("Loading file metadata"))')
            assert page.locator('#accessible [role=gridcell]').count() == 0
            page.locator('#table-mode').select_option('3')
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
            page.locator('#selection-mode').select_option('1')
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
            page.wait_for_timeout(80)
            signature = 'JSON.stringify(Array.from({length:cuiGallery.exports.gallery_count()},(_,i)=>Array.from({length:12},(_,f)=>cuiGallery.exports.gallery_command(i,f))))'
            initial_signature = page.evaluate(signature)
            page.wait_for_timeout(200)
            assert page.evaluate(signature) != initial_signature
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.exports.gallery_redraw()===0')
            initial_signature = page.evaluate(signature)
            page.wait_for_timeout(100)
            assert page.evaluate(signature) == initial_signature
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
            page.locator('#input-mode').select_option('1')
            page.wait_for_function('document.querySelector("#accessible [role=textbox]")?.getAttribute("aria-readonly")==="true"')
            length = page.evaluate('cuiGallery.exports.gallery_input_selection(2)')
            canvas.focus()
            page.keyboard.type('R')
            page.wait_for_timeout(50)
            assert page.evaluate('cuiGallery.exports.gallery_input_selection(2)') == length
            page.locator('#input-mode').select_option('2')
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
            page.locator('#table-pin').select_option('1')
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
            print('PASS: 78 catalog entries, 54 real WASM stories, controlled form validation, surface focus/dismissal, split-button actions, table interactions, themes, pending status, search, 390px layout, /cui/ hosting')
    finally:
        server.shutdown()
        server.server_close()

if __name__ == '__main__':
    main()
