#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Verify full-width list paint and input in the real WASM gallery."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading

from playwright.sync_api import sync_playwright
from test_touch_components import Audit


class Handler(SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass


def press(audit, target, touch, rx=.5):
    point = audit.point(target, rx=rx)
    if touch:
        audit.dispatch('touchStart', [point])
        audit.dispatch('touchEnd', [])
    else:
        audit.page.mouse.click(point['x'], point['y'])
        audit.page.wait_for_function('!cuiGallery.exports.gallery_error()')


def modal(audit, visible):
    audit.page.wait_for_function('(visible)=>cuiGallery.exports.gallery_modal()===visible',
                                 arg=int(visible))


def row_span(row, panel, left, right):
    x, _, width, _ = row['f'][1:5]
    px, _, panel_width, _ = panel['f'][1:5]
    assert abs(x - px - left) < 1 and abs(px + panel_width - x - width - right) < 1, (
        row['name'], row['f'][1:5], panel['f'][1:5], left, right)


def row_paint(audit, row):
    audit.page.wait_for_function('''bounds=>{
        const e=cuiGallery.exports;
        return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>
            e.gallery_command(i,0)===0 && bounds.every((edge,j)=>
                Math.abs(e.gallery_command(i,j+1)-edge)<.5));
    }''', arg=row['f'][1:5])


def drawn_label(audit, label):
    audit.page.wait_for_function('''label=>{
        const e=cuiGallery.exports,d=new TextDecoder();
        return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>
            e.gallery_command(i,0)===2 &&
            d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
                                   e.gallery_command_length(i)))===label);
    }''', arg=label)


def select(audit, touch):
    audit.route('Select')
    trigger = audit.target('Delivery', 1)
    assert audit.page.evaluate('''([top,height])=>{
        const e=cuiGallery.exports,d=new TextDecoder();
        return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>
            e.gallery_command(i,0)===2 &&
            d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
                                   e.gallery_command_length(i)))==='Select a service' &&
            Math.abs(e.gallery_command(i,2)+e.gallery_command(i,4)/2-top-height/2)<1);
    }''', [trigger['f'][2], trigger['f'][4]])
    press(audit, trigger, touch)
    modal(audit, True)
    panel = audit.target('Delivery', 13)
    standard = audit.target('Standard', 14)
    priority = audit.target('Priority', 14)
    row_span(standard, panel, 4, 4)
    row_span(priority, panel, 4, 4)
    row_paint(audit, standard)
    if touch:
        press(audit, priority, touch, rx=.97)
    else:
        audit.page.keyboard.press('ArrowDown')
        row_paint(audit, priority)
        audit.page.keyboard.press('Enter')
    modal(audit, False)
    drawn_label(audit, 'Priority')
    press(audit, audit.target('Delivery', 1), touch)
    modal(audit, True)
    assert audit.target('Priority', 14)['f'][11] == 1
    audit.page.keyboard.press('Escape')
    modal(audit, False)


def combobox(audit, touch):
    audit.route('Combobox')
    field = audit.target('Find a fruit', 6)
    press(audit, field, touch, rx=.1)
    modal(audit, True)
    panel = audit.target('Find a fruit', 13)
    first = audit.target('Apple', 14)
    second = audit.target('Apricot', 14)
    row_span(first, panel, 4, 4)
    row_span(second, panel, 4, 4)
    row_paint(audit, first)
    if touch:
        press(audit, second, touch, rx=.97)
    else:
        audit.page.keyboard.press('ArrowDown')
        row_paint(audit, second)
        audit.page.keyboard.press('Enter')
    modal(audit, False)
    press(audit, audit.target('Find a fruit', 6), touch, rx=.1)
    modal(audit, True)
    assert audit.target('Apricot', 14)['f'][11] == 1
    audit.page.keyboard.press('Escape')
    modal(audit, False)


def command(audit, touch):
    audit.route('Command')
    audit.configure('command-modal', True)
    # Capture the first accessible frame. Typing into the palette would otherwise
    # trigger another layout and conceal a stale opening width.
    audit.page.evaluate('''()=>{
        window.commandOpening = null;
        const e=cuiGallery.exports;
        const observer=new MutationObserver(()=>{
            if(e.gallery_command_palette_stats(8)!==1)return;
            const ids=Array.from({length:e.gallery_semantic_count()},(_,i)=>i);
            const width=role=>e.gallery_semantic(ids.find(i=>e.gallery_semantic(i,0)===role),3);
            window.commandOpening={dialog:width(17),list:width(13),row:width(14)};
            observer.disconnect();
        });
        observer.observe(document.querySelector('#accessible'),{childList:true,subtree:true});
    }''')
    press(audit, audit.target('Open command palette', 1), touch)
    audit.page.wait_for_function('window.commandOpening !== null')
    opening = audit.page.evaluate('window.commandOpening')
    assert opening['dialog'] > 0 and abs(opening['dialog'] - opening['list'] - 22) < 1, opening
    assert opening['row'] > 0 and abs(opening['list'] - opening['row'] - 18) < 1, opening
    dialog = audit.target('Command palette', 17)
    field = audit.target('Command palette', 6)
    audit.page.keyboard.insert_text('\u00a0ACCÈS\u3000')
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(1)===1')
    audit.target('Copy path', 14)  # The French alias is a keyword, not the row label.
    audit.page.keyboard.press('Escape')
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(5)===0')
    audit.page.keyboard.type('sw')
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(1)===1')
    row = audit.target('Switch workspace', 14)
    panel = audit.target(role=13)
    assert abs(dialog['f'][3] - panel['f'][3] -
               2 * (panel['f'][1] - dialog['f'][1])) < 1, (dialog, panel)
    assert field['f'][1] + field['f'][3] <= panel['f'][1] + panel['f'][3] + 1
    row_span(row, panel, 4, 14)
    row_paint(audit, row)
    if touch:
        press(audit, row, touch, rx=.97)
    else:
        audit.page.keyboard.press('Enter')
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(2)===1')
    audit.page.keyboard.press('Escape')  # Clear the query first.
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(5)===0')
    audit.page.keyboard.press('Escape')
    audit.page.wait_for_function('cuiGallery.exports.gallery_command_palette_stats(8)===0')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    handler = partial(Handler, directory=str(args.directory.resolve()))
    with ThreadingHTTPServer(('127.0.0.1', 0), handler) as server:
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            with sync_playwright() as p:
                browser = p.chromium.launch(headless=True)
                for touch in (False, True):
                    audit = Audit(browser, f'http://127.0.0.1:{server.server_port}')
                    if touch:
                        audit.page.set_viewport_size({'width': 390, 'height': 844})
                    for check in (select, combobox, command):
                        check(audit, touch)
                        assert audit.page.evaluate('cuiGallery.exports.gallery_error()') == 0
                        assert not audit.errors, audit.errors
                        print(f'PASS {check.__name__} row paint and selection touch={touch}', flush=True)
                    audit.page.context.close()
                browser.close()
        finally:
            server.shutdown()


if __name__ == '__main__':
    main()
