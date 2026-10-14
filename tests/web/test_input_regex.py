# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Chromium proxy/CDP -> compiled CUI Input -> pinned regex provider."""
import argparse
from functools import partial
from http.server import ThreadingHTTPServer,SimpleHTTPRequestHandler
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
server=ThreadingHTTPServer(('127.0.0.1',0),partial(SimpleHTTPRequestHandler,directory=str(a.fixture)))
threading.Thread(target=server.serve_forever,daemon=True).start()
with sync_playwright() as pw:
 browser=pw.chromium.launch(executable_path='/home/matteo/.cache/ms-playwright/chromium-1208/chrome-linux64/chrome',headless=True)
 page=browser.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe?.ready')
 def value():return page.evaluate('probe.value()')
 page.keyboard.type('12');assert value()=='12' and page.evaluate('probe.painted("12")')
 page.keyboard.type('x');assert value()=='12' and page.evaluate('probe.flags()&2')
 page.keyboard.press('Control+a');page.keyboard.press('Backspace');assert value()==''
 page.keyboard.press('Control+z');assert value()=='12';page.keyboard.press('Control+y');assert value()==''
 page.evaluate('probe.seed("bad")');page.keyboard.type('x');assert value()=='badx'
 page.evaluate('probe.seed("12");probe.selectAll()')
 cdp=page.context.new_cdp_session(page)
 cdp.send('Input.imeSetComposition',{'text':'56','selectionStart':2,'selectionEnd':2})
 page.wait_for_function('probe.events().includes("update")');assert value()=='12' and page.evaluate('probe.painted("56")')
 cdp.send('Input.imeSetComposition',{'text':'x','selectionStart':1,'selectionEnd':1})
 page.wait_for_function('probe.flags()&2');assert value()=='12' and page.evaluate('probe.painted("56")')
 cdp.send('Input.insertText',{'text':'56'});page.wait_for_function('probe.value()==="56"');assert page.evaluate('probe.events().filter(x=>x==="commit").length')==1
 page.keyboard.press('Control+z');assert value()=='12'
 page.evaluate('probe.selectAll()');cdp.send('Input.imeSetComposition',{'text':'x','selectionStart':1,'selectionEnd':1});cdp.send('Input.insertText',{'text':'x'});assert value()=='12' and not page.evaluate('probe.flags()&4')
 page.keyboard.type('7');assert value()=='7'
 page.evaluate('probe.masked(true)');assert page.locator('#cui-secure-input').get_attribute('type')=='password';assert page.evaluate('probe.painted("•")&&!probe.painted("7")')
 page.keyboard.type('x');assert value()=='7';page.evaluate('probe.masked(false)')
 page.evaluate('probe.readonly(true)');assert page.locator('#cui-secure-input').count()==0 and value()=='7';page.evaluate('probe.readonly(false)')
 assert page.evaluate('probe.pattern("^[\\\\p{Greek}]*$")')==0
 page.evaluate('probe.seed("")');cdp.send('Input.insertText',{'text':'ΔΣ'});assert value()=='ΔΣ';page.keyboard.type('x');assert value()=='ΔΣ'
 assert not errors,errors
 print(f'PASS Chromium {browser.version}: actual CUI Input typing/rejection/healing, Undo/Redo, CDP IME/update/commit-once/rejected-host-end, masking, readonly, Unicode provider')
 browser.close()
server.shutdown()
