#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0", "pillow==11.3.0"]
# ///
"""Capture real CUI WASM presentation and compose original website/share media."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from PIL import Image, ImageDraw, ImageFont
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=ROOT / 'build/web')
    parser.add_argument('--chromium')
    parser.add_argument('--font', type=Path, default=Path('/usr/share/fonts/noto/NotoSans-Regular.ttf'))
    args = parser.parse_args()
    output = ROOT / 'web/assets'
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(SimpleHTTPRequestHandler, directory=str(args.directory)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as p:
            options = {'headless': True}
            if args.chromium:
                options['executable_path'] = args.chromium
            browser = p.chromium.launch(**options)
            page = browser.new_page(viewport={'width': 1280, 'height': 1000}, device_scale_factor=2)
            page.goto(f'http://127.0.0.1:{server.server_port}/#Resizable')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Resizable"')
            page.evaluate('document.fonts.ready')
            for mode in ['light', 'dark']:
                page.evaluate("mode=>{const input=document.querySelector('#theme');input.value=mode==='dark'?'1':'0';input.dispatchEvent(new Event('change'))}", mode)
                page.wait_for_function("mode=>document.body.classList.contains('dark')===(mode==='dark')", arg=mode)
                page.wait_for_timeout(100)
                page.locator('#preview').screenshot(path=str(output / f'cui-workspace-{mode}.png'))
            browser.close()
    finally:
        server.shutdown()
        server.server_close()
    # A real product render, original composition, no upstream screenshot/branding.
    card = Image.new('RGB', (1200, 630))
    draw = ImageDraw.Draw(card)
    for y in range(630):
        t = y / 629
        draw.line((0, y, 1199, y), fill=tuple(round(a+(b-a)*t) for a,b in zip((232,245,240),(247,249,245))))
    heading = ImageFont.truetype(str(args.font), 64)
    subtitle = ImageFont.truetype(str(args.font), 26)
    small = ImageFont.truetype(str(args.font), 20)
    draw.text((60, 35), 'CUI', font=heading, fill='#193b31')
    draw.text((235, 57), 'Native UI. Portable C11.', font=subtitle, fill='#37534a')
    screenshot = Image.open(output / 'cui-workspace-light.png').convert('RGBA')
    screenshot.thumbnail((1080, 405), Image.Resampling.LANCZOS)
    x, y = (1200-screenshot.width)//2, 160
    draw.rounded_rectangle((x-2,y-2,x+screenshot.width+2,y+screenshot.height+2), radius=14, fill='#b7cfc2')
    background = Image.new('RGBA', screenshot.size, '#f4f7f5')
    background.alpha_composite(screenshot)
    card.paste(background.convert('RGB'), (x,y))
    draw.text((60, 568), 'Components, examples and docs • C + WebAssembly', font=small, fill='#37534a')
    draw.text((842, 568), 'matteo.merola.co/cui', font=small, fill='#37534a')
    card.save(output / 'cui-social.png', optimize=True)
    print('Captured real CUI workspace previews and 1200×630 social card.')


if __name__ == '__main__':
    main()
