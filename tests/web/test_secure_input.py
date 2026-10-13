#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Chromium CDP composition against the standalone CUI WASM Input fixture."""
import argparse
import functools
import http.server
from pathlib import Path
import shutil
import subprocess
import sys
import threading
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);parser.add_argument('--chromium');args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    sys.path.insert(0,str(ROOT/'scripts'));import build_web
    sources=[str(ROOT/s) for s in build_web.SOURCES if s not in ['web/gallery.c','examples/composed_apps/app.c']]
    exports=['probe_init','probe_command','probe_frame','probe_focus','probe_options','probe_config','probe_text','probe_composition','probe_key','probe_value','probe_length','probe_draw','probe_draw_length','probe_count','probe_active','probe_key_id','probe_fixed','probe_error','probe_capacity_error','probe_selection','probe_multiline','malloc','free']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*sources,str(ROOT/'tests/web/secure_input_fixture/probe.c'),'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={v}' for v in exports],'-o',str(out/'probe.wasm')],check=True)
    shutil.copy(ROOT/'tests/web/secure_input_fixture/index.html',out/'index.html');shutil.copy(ROOT/'web/secure_input.js',out/'secure_input.js')
    class Handler(http.server.SimpleHTTPRequestHandler):
      def log_message(self,*args):pass
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),functools.partial(Handler,directory=str(out)));threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
      with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=args.chromium,headless=True);page=browser.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
        page.goto(f'http://127.0.0.1:{server.server_port}/');page.wait_for_function('window.proof && document.activeElement.id==="cui-secure-input"')
        session=page.context.new_cdp_session(page)
        assert page.locator('#cui-secure-input').get_attribute('type')=='password'
        page.keyboard.insert_text('aé😀z');page.wait_for_timeout(80)
        page.evaluate('proof.e.probe_selection(1,7);proof.e.probe_composition(0,0,0,0,0);proof.frame()')
        assert page.evaluate('proof.value()')=='aé😀z'
        assert page.evaluate('proof.draws().join("")')=='••'
        assert page.evaluate('proof.e.probe_active()')==1
        page.evaluate('proof.e.probe_composition(3,0,0,0,0);proof.frame()')
        assert page.evaluate('proof.draws().join("")')=='••••'
        page.keyboard.press('Control+a');page.wait_for_timeout(50)
        page.keyboard.insert_text('secret');page.wait_for_timeout(80);assert page.evaluate('proof.value()')=='secret'

        pixels=page.locator('canvas').screenshot()
        assert page.evaluate("(()=>{let p=document.querySelector('canvas').getContext('2d').getImageData(0,0,500,150).data,n=0;for(let i=0;i<p.length;i+=4)if(p[i]<100&&p[i+1]<100&&p[i+2]<100&&p[i+3]>200)n++;return n})()")>30
        page.keyboard.press('Control+a');page.wait_for_timeout(50)
        page.keyboard.insert_text('foobar');page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='foobar' and page.locator('canvas').screenshot()==pixels
        page.keyboard.press('Control+z');page.wait_for_timeout(80);assert page.evaluate('proof.value()')=='secret'
        assert page.evaluate('proof.draws().join("").includes("secret")') is False
        page.keyboard.press('Control+a');page.wait_for_timeout(50)
        session.send('Input.imeSetComposition',{'text':'😀é','selectionStart':2,'selectionEnd':3});page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='secret'
        assert '••' in page.evaluate('proof.draws().join("")')
        assert page.evaluate('proof.e.probe_active()')==1
        session.send('Input.insertText',{'text':'😀é'});page.wait_for_timeout(100)
        assert page.evaluate('proof.value()')=='😀é',page.evaluate('({value:proof.value(),active:proof.e.probe_active(),proxy:proof.bridge.element?.value,events:window.events})')
        assert page.evaluate('proof.e.probe_active()')==0
        assert page.locator('#cui-secure-input').input_value()==''
        page.keyboard.press('Control+z');page.wait_for_timeout(80);assert page.evaluate('proof.value()')=='secret'
        session.send('Input.imeSetComposition',{'text':'取消','selectionStart':2,'selectionEnd':2});page.wait_for_timeout(50)
        session.send('Input.imeSetComposition',{'text':'','selectionStart':0,'selectionEnd':0})
        # A cancellation must not swallow the next native insertion, even without
        # a delay to let the composition-end duplicate guard expire.
        session.send('Input.insertText',{'text':'x'});page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='x',page.evaluate('({value:proof.value(),active:proof.e.probe_active(),events:window.events})')
        page.keyboard.press('Control+z');page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='secret'
        page.evaluate('proof.e.probe_fixed(1);proof.frame()')
        page.keyboard.press('Control+a');page.wait_for_timeout(50)
        session.send('Input.imeSetComposition',{'text':'candidate exceeds ten scalars','selectionStart':2,'selectionEnd':3});page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='secret'
        session.send('Input.insertText',{'text':'candidate exceeds ten scalars'});page.wait_for_timeout(80)
        assert page.evaluate('proof.e.probe_error()===proof.e.probe_capacity_error()')
        assert page.evaluate('proof.value()')=='secret' and page.evaluate('proof.e.probe_active()')==0
        page.keyboard.insert_text('x');page.wait_for_timeout(80);assert page.evaluate('proof.value()')=='x'
        page.keyboard.press('Control+z');page.wait_for_timeout(80);assert page.evaluate('proof.value()')=='secret'
        page.evaluate('proof.e.probe_fixed(0);proof.frame()')
        page.evaluate('proof.options(0,1,0,0)');assert page.locator('#cui-secure-input').get_attribute('type')=='text';assert page.locator('#cui-secure-input').input_value()==''
        page.evaluate('proof.options(1,2,0,0)');assert page.locator('#cui-secure-input').get_attribute('autocomplete')=='new-password'
        page.evaluate('proof.options(1,2,1,0)');assert page.locator('#cui-secure-input').count()==0
        page.evaluate('proof.options(1,2,0,0);proof.focus()');page.wait_for_timeout(50)
        session.send('Input.imeSetComposition',{'text':'blur secret','selectionStart':3,'selectionEnd':3});page.wait_for_timeout(50)
        page.locator('#blur').click();page.wait_for_timeout(80);assert page.locator('#cui-secure-input').count()==0;assert page.evaluate('proof.value()')=='secret'
        page.evaluate('proof.focus();proof.multiline(1)');page.wait_for_timeout(50)
        assert page.locator('textarea#cui-secure-input').count()==1
        multiline_generation=page.evaluate('proof.e.probe_config(3)')
        page.keyboard.type('paragraph');page.keyboard.press('Shift+Enter');page.wait_for_timeout(80)
        assert page.evaluate('proof.value()')=='paragraph\n'
        page.evaluate('proof.multiline(0)');page.wait_for_timeout(50)
        assert page.locator('input#cui-secure-input').count()==1
        assert page.evaluate('proof.e.probe_config(3)')>multiline_generation
        assert not errors,errors;browser.close();print('secure Input actual WASM/CDP PASS')
    finally:server.shutdown()
if __name__=='__main__':main()
