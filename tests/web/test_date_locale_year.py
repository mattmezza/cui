#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Strict locale/year-grid checks with actual mouse and CDP touch actions."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

LOCALES=[('Choose year','Previous year','February','02/29/1980'),
 ('Choose year','Previous year','February','29/02/1980'),
 ('Scegli anno','Anno precedente','febbraio','29/02/1980'),
 ('Choisir année','Année précédente','février','29/02/1980'),
 ('Jahr wählen','Vorheriges Jahr','Februar','29.02.1980'),
 ('Elegir año','Año anterior','febrero','29/02/1980'),
 ('年を選択','前年','2月','1980/02/29')]

def activate(a,target,touch):
    if touch:a.tap(target)
    else:
        point=a.point(target);a.page.mouse.click(point['x'],point['y']);a.page.wait_for_timeout(80)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,default=Path('build/web'))
    p.add_argument('--url');p.add_argument('--chromium')
    p.add_argument('--output',type=Path,default=Path('/tmp/cui-date-locale-year'))
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):pass
    server=None
    if not args.url:
        server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
        threading.Thread(target=server.serve_forever,daemon=True).start()
        args.url=f'http://127.0.0.1:{server.server_port}'
    results=[]
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            for family in ('Calendar','DatePicker'):
                for kind,(header,previous,february,formatted) in enumerate(LOCALES):
                    for touch in (False,True):
                        a=Audit(browser,args.url);a.route(family);a.configure('date-locale',kind)
                        if family=='DatePicker':
                            trigger=a.target(role=1,predicate=lambda s: s['name'].endswith('2026') or s['name'].startswith('2026'))
                            activate(a,trigger,touch)
                        activate(a,a.target(header,1),touch)
                        years=[s for s in a.snapshot() if s['f'][0]==1 and s['name'].isdigit() and len(s['name'])==4]
                        assert len(years)==20,'Year view must expose bounded 20-year grid'
                        if kind==2 and not touch:a.page.screenshot(path=str(args.output/f'{family}-year-grid.png'))
                        assert max(s['f'][3] for s in years)-min(s['f'][3] for s in years)<1,'Year grid columns unequal'
                        group=a.target(['Calendar','Calendar','Calendario','Calendrier','Kalender','Calendario','カレンダー'][kind])
                        left=min(s['f'][1] for s in years);right=max(s['f'][1]+s['f'][3] for s in years)
                        assert abs((left+right)/2-(group['f'][1]+group['f'][3]/2))<1,'Year grid is not centered'

                        for _ in range(3):activate(a,a.target(previous,1),touch)
                        activate(a,a.target('1980',1),touch)
                        assert any(s['name']==february for s in a.snapshot()),'Localized month grid missing'
                        activate(a,a.target(february,1),touch)
                        weeks=[['Su','Mo','Tu','We','Th','Fr','Sa'],['Mo','Tu','We','Th','Fr','Sa','Su'],
                            ['lu','ma','me','gi','ve','sa','do'],['lu','ma','me','je','ve','sa','di'],
                            ['Mo','Di','Mi','Do','Fr','Sa','So'],['lu','ma','mi','ju','vi','sá','do'],
                            ['日','月','火','水','木','金','土']]
                        painted=a.texts()
                        assert any(painted[n:n+7]==weeks[kind] for n in range(len(painted))), 'Locale weekday order incorrect'
                        activate(a,a.target(formatted,1),touch)
                        assert a.page.evaluate('cuiGallery.exports.gallery_date_value()')==19800229,'Actual date model incorrect'
                        if family=='DatePicker':
                            assert a.page.evaluate('cuiGallery.exports.gallery_modal()')==0,'Choosing date did not close popup'
                            assert a.target(formatted,1),'Localized accessible trigger missing'
                            assert formatted in a.texts(),'Localized trigger paint missing'
                            activate(a,a.target(formatted,1),touch)
                            activate(a,a.target(header,1),touch)
                            a.page.keyboard.press('Escape');a.page.wait_for_timeout(80)
                            assert a.page.evaluate('cuiGallery.exports.gallery_modal()')==1,'First Escape should return to days'
                            a.page.keyboard.press('Escape');a.page.wait_for_timeout(80)
                            assert a.page.evaluate('cuiGallery.exports.gallery_modal()')==0,'Second Escape should dismiss popup'
                        if kind==2 and not touch:a.page.screenshot(path=str(args.output/f'{family}-italian.png'))
                        assert not a.errors,a.errors
                        results.append({'family':family,'locale':kind,'touch':touch,'date':formatted})
                        print(f'PASS {family} locale={kind} touch={touch}',flush=True)
                        a.page.close()
            browser.close()
        (args.output/'report.json').write_text(json.dumps(results,indent=2)+'\n')
        print(f'{len(results)}/28 date locale/year cases passed')
    finally:
        if server:server.shutdown()
if __name__=='__main__':main()
