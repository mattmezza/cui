#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Capture the current composed C/WASM application for the README."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=ROOT / 'build/web')
    parser.add_argument('--chromium')
    args = parser.parse_args()
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(SimpleHTTPRequestHandler, directory=str(args.directory)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1700, 'height': 1100}, device_scale_factor=2,
                                    color_scheme='light', reduced_motion='reduce')
            page.goto(f'http://127.0.0.1:{server.server_port}/#examples/northstar')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Northstar workspace"')
            page.locator('#palette-trigger').click()
            page.locator('[data-palette="aurora-light"]').click()
            page.wait_for_function('cuiGallery.themeChoice==="aurora-light"')
            page.evaluate('document.fonts.ready')
            page.evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))')
            assert page.evaluate('cuiGallery.exports.gallery_error()===0')
            destination = ROOT / 'docs/screenshots/readme-workspace.png'
            page.locator('#preview').screenshot(path=str(destination))
            browser.close()
            print(destination)
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
