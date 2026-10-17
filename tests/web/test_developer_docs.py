#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Verify application-building discovery and version-matched API exports headlessly."""
import argparse
import hashlib
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
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True, **({'executable_path': args.chromium} if args.chromium else {}))
            page = browser.new_page(viewport={'width': 1280, 'height': 960}, permissions=['clipboard-read', 'clipboard-write'])
            errors = []
            page.on('pageerror', lambda error: errors.append(str(error)))
            base = f'http://127.0.0.1:{server.server_port}/'
            page.goto(base + 'docs/layout_guide/')
            page.wait_for_function('window.cuiGallery?.documents && !document.querySelector("#docs-page").hidden')
            assert 'layout' in page.locator('#docs-title').inner_text().lower()
            page.locator('#search').fill('star disappeared')
            assert page.locator('#docs-nav a').filter(has_text='layout').count() > 0
            page.locator('#search').fill('blank icon')
            assert page.locator('#docs-nav a').filter(has_text='Icon').count() > 0
            markdown_url = page.locator('#docs-markdown').get_attribute('href')
            response = page.request.get(base + markdown_url.removeprefix('./'))
            assert response.ok and response.text().startswith('# ')
            assert page.locator('link[rel=alternate][type="text/markdown"]').count() == 1
            info = page.request.get(base + 'build-info.json').json()
            archive = page.request.get(base + 'downloads/cui-native.tar.gz')
            assert archive.ok
            assert hashlib.sha256(archive.body()).hexdigest() == info['downloads']['downloads/cui-native.tar.gz']['sha256']
            api = page.request.get(base + 'api-index.json').json()
            assert api['source_revision'] == info['source_revision']
            assert any(symbol['name'] == 'cui_text_ellipsis' for symbol in api['symbols'])
            header = page.request.get(base + 'headers/cui.h')
            assert header.ok and 'cui_first_diagnostic' in header.text()
            page.locator('#search').fill('')
            page.evaluate('location.hash="Icon"')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Icon"')
            page.wait_for_function('document.querySelector("#icon-catalog-results").children.length>0')
            page.wait_for_function('document.querySelector("#component-declarations").textContent.includes("cui_icon_asset")')
            page.locator('#icon-catalog-search').fill('clock-3')
            assert '0 matching' in page.locator('#icon-catalog-count').inner_text()
            page.locator('#icon-catalog-embedded').uncheck()
            assert 'Requires loader' in page.locator('#icon-catalog-results').inner_text()
            page.get_by_role('button', name='Copy clock-3 C example', exact=True).click()
            page.wait_for_function('document.querySelector("#icon-catalog-results button").textContent==="Copied"')
            copied = page.evaluate('navigator.clipboard.readText()')
            assert 'cui_string("clock-3")' in copied and 'CUI_ICON_ALL_ASSETS=1' in copied
            page.locator('#icon-catalog-search').fill('mail')
            page.locator('#icon-catalog-embedded').check()
            assert 'Embedded by default' in page.locator('#icon-catalog-results').inner_text()
            assert not errors, errors
            browser.close()
            print('Developer docs: symptom search, exact headers, revision metadata and icon availability passed.')
    finally:
        server.shutdown()


if __name__ == '__main__':
    main()
