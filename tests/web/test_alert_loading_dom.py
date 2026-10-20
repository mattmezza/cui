#!/usr/bin/env python3
"""Actual compiled Core snapshots through the gallery's unchanged DOM translator.

No WASM, spoken output or live-AT delivery claim. Uses installed pinned Playwright
and Chromium only; never installs dependencies or launches a headed browser.
"""
import argparse
import json
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--example', type=Path, required=True)
    parser.add_argument('--chromium', type=Path, default=Path.home() / '.cache/ms-playwright/chromium-1208/chrome-linux64/chrome')
    args = parser.parse_args()
    frames = json.loads(subprocess.check_output([str(args.example.resolve())], text=True))
    source = (Path(__file__).resolve().parents[2] / 'web/gallery.js').read_text()
    # Execute the production role mapping and translator verbatim, rather than
    # duplicating their implementation in a test-only DOM renderer.
    translator = source[source.index('const roles='):source.index('function fuzzyScore(')]
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, executable_path=str(args.chromium))
        page = browser.new_page()
        errors = []
        page.on('pageerror', lambda error: errors.append(str(error)))
        page.set_content('<div id="accessible"></div>' + ''.join(f'<i id="{i}"></i>' for i in ['scrollbar-policy', 'scrollbar-width', 'scrollbar-radius', 'scrollbar-track-control']))
        page.add_script_tag(content='''
            const $=id=>document.getElementById(id);
            let selected={name:'LoadingRegion'},semanticStory='',semanticNodes=new Map(),rows=[];
            const stringAt=s=>s,requestFrame=()=>{},keyIdentity=()=>0,mods=()=>0;
            const wasm={
              gallery_semantic_count:()=>rows.length,
              gallery_semantic:(i,n)=>n===0?rows[i].role:n===14?Number(rows[i].busy):0,
              gallery_semantic_id:i=>rows[i].id,
              gallery_semantic_parent:i=>rows[i].parent,
              gallery_semantic_controls:()=>0,gallery_semantic_labelled_by:()=>0,
              gallery_semantic_name:i=>rows[i].name,gallery_semantic_length:i=>rows[i].name.length,
              gallery_semantic_description:()=>'',gallery_semantic_description_length:()=>0
            };
        ''' + translator + '\nwindow.apply=next=>{rows=next;syncSemantics();};')
        for i, frame in enumerate(frames):
            page.evaluate('apply', frame)
            region = page.locator('#accessible [role=region]')
            assert region.count() == 1 and region.get_attribute('aria-label') == 'Records'
            assert region.get_attribute('aria-busy') == str(i < 2).lower()
            assert page.locator('#cui-semantic-20,#cui-semantic-21,#cui-semantic-22').count() == 0
            if i < 3:
                assert page.locator('#accessible [role=alert]').count() == 0
                assert region.locator('[role=status]').get_attribute('aria-label') == ('Loading records' if i < 2 else 'Records loaded')
                if i == 0:
                    page.evaluate('window.savedStatus=document.querySelector("[role=status]")')
                if i == 1:
                    assert page.evaluate('savedStatus===document.querySelector("[role=status]")')
            elif i == 6:
                assert region.locator('[role=alert],button,[role=status]').count() == 0
            else:
                alert = region.locator('[role=alert]')
                assert alert.count() == 1 and alert.get_attribute('aria-label') == ('Records unavailable' if i < 5 else 'Retry failed')
                assert alert.get_attribute('tabindex') is None
                assert alert.locator('button').count() == 2
                assert alert.locator('button').all_text_contents() == ['Retry records', 'Dismiss']
                if i == 3:
                    page.evaluate('window.savedAlert=document.querySelector("[role=alert]")')
                if i in (4, 5):
                    assert page.evaluate('savedAlert===document.querySelector("[role=alert]")')
                if i == 7:
                    assert page.evaluate('savedAlert!==document.querySelector("[role=alert]")')
        assert not errors, errors
        browser.close()
    print('Core snapshot -> production gallery DOM identity/name/busy/subtree transitions PASS')


if __name__ == '__main__':
    main()
