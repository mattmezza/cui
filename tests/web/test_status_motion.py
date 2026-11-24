# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM scheduling/pixel/semantics checks in pinned Chromium 1208."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


def skeleton_shapes(page):
    return page.evaluate("""() => {
        const e = cuiGallery.exports, canvas = document.querySelector('canvas');
        const ctx = canvas.getContext('2d'), shapes = {};
        for (let i = 0; i < e.gallery_count(); i++) {
            const f = n => e.gallery_command(i, n);
            const [x, y, w, h, radius] = [1, 2, 3, 4, 9].map(f);
            if (f(0) !== 0 || !([10, 16, 28].includes(h) && radius === 4 ||
                                  h === 40 && radius === 20)) continue;
            const pixel = (px, py) => Array.from(ctx.getImageData(
                Math.floor(px), Math.floor(py), 1, 1).data);
            shapes[h] = {x, y, w, h, radius, fill: [5, 6, 7, 8].map(f),
                         center: pixel(x + w / 2, y + h / 2), corner: pixel(x + 1, y + 1),
                         outside: pixel(x - 2, y + 1)};
        }
        return shapes;
    }""")


def skeleton_preview(page):
    assert page.evaluate('cuiGallery.themeChoice') == 'aurora-light'
    assert page.locator('#disabled').is_hidden()
    code = page.locator('#snippet').text_content()
    assert 'cui_skeleton_present' in code and '.busy = loading' in code
    page.locator('#copy-snippet').click()
    page.wait_for_function('document.querySelector("#copy-snippet").textContent === "Copied"')
    assert page.evaluate('navigator.clipboard.readText()') == code
    region = page.locator('#accessible [role=region][aria-label="Profile"]')
    assert region.get_attribute('aria-busy') == 'true'
    assert region.locator('[tabindex],button,[role]').count() == 0
    desktop = skeleton_shapes(page)
    assert list(desktop) == ['10', '16', '28', '40'], desktop
    # Aurora Light has an explicit Skeleton token, distinct from theme.secondary.
    for shape in desktop.values():
        assert shape['fill'] == [241, 245, 249, 255] and shape['center'] == shape['fill'], shape
    circle = desktop['40']
    assert circle['w'] == 40 and circle['corner'] == circle['outside'] != circle['center'], circle

    frame = page.evaluate('cuiGallery.frame')
    page.set_viewport_size({'width': 390, 'height': 844})
    page.wait_for_function('(frame) => cuiGallery.frame > frame', arg=frame)
    mobile = skeleton_shapes(page)
    assert mobile.keys() == desktop.keys(), mobile
    assert mobile['40']['w'] == 40
    for h in ['10', '16', '28']:
        assert 0 < mobile[h]['w'] < desktop[h]['w'], (mobile, desktop)
    row_space = mobile['28']['w'] - 40 - 12
    assert abs(mobile['16']['w'] - .65 * row_space) < 2, mobile
    assert abs(mobile['10']['w'] - .40 * row_space) < 2, mobile
    assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')

    appearance = page.locator('#skeleton-appearance')
    for mode in ['1', '2']:
        frame = page.evaluate('cuiGallery.frame')
        appearance.select_option(mode)
        page.wait_for_function('(frame) => cuiGallery.frame > frame', arg=frame)
        expected = [241, 245, 249, 127] if mode == '1' else page.evaluate(
            '[0,1,2,3].map(channel => cuiGallery.exports.gallery_theme_channel(2, channel))')
        for shape in skeleton_shapes(page).values():
            assert shape['fill'] == expected, shape
            assert shape['outside'][3] == 255
            alpha = expected[3] / 255
            composite = [round(fg * alpha + bg * (1 - alpha))
                         for fg, bg in zip(expected[:3], shape['outside'][:3])] + [255]
            assert all(abs(got - want) <= 1 for got, want in zip(shape['center'], composite)), shape

    # Loaded content stops the animation even when motion is allowed.
    page.emulate_media(reduced_motion='no-preference')
    page.wait_for_function('cuiGallery.exports.gallery_next_wake() > 0')
    page.locator('#skeleton-state').select_option('0')
    page.wait_for_function('document.querySelector("#accessible [role=status][aria-label=\'Profile loaded\']")')
    assert region.get_attribute('aria-busy') == 'false'
    assert appearance.is_hidden()
    assert not {'10', '16', '28'} & skeleton_shapes(page).keys()
    assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
    idle = page.evaluate('cuiGallery.frame')
    page.wait_for_timeout(180)
    assert page.evaluate('cuiGallery.frame') == idle
    page.locator('#skeleton-state').select_option('1')
    page.wait_for_function('cuiGallery.exports.gallery_next_wake() > 0')
    assert region.get_attribute('aria-busy') == 'true'
    assert region.locator('[role=status]').count() == 0
    assert appearance.is_visible()
    page.set_viewport_size({'width': 1200, 'height': 900})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path,default=Path('build/web'))
    parser.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page(viewport={'width':1200,'height':900},
                                  permissions=['clipboard-read', 'clipboard-write'])
            page.add_init_script("localStorage.setItem('cui.theme', 'aurora-light')")
            errors=[]; page.on('pageerror',lambda e:errors.append(str(e)))
            for family in ['Spinner','Skeleton']:
                page.goto(f'http://127.0.0.1:{server.server_port}/#{family}')
                page.wait_for_function('(name)=>window.cuiGallery?.renderedComponent===name',arg=family)
                page.wait_for_timeout(80)
                assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
                assert page.evaluate('cuiGallery.exports.gallery_next_wake()')>0
                # Inspect actual canvas pixels as well as the SVG mask/rectangle.
                assert page.evaluate('''()=>{const c=document.querySelector('canvas'),d=c.getContext('2d').getImageData(0,0,c.width,c.height).data;return d.some((v,i)=>i%4!==3&&v>0)}''')
                moving_pixels=page.locator('canvas').screenshot()
                before=page.evaluate('cuiGallery.frame')
                page.wait_for_function('(before)=>cuiGallery.frame>before+2',arg=before)
                page.wait_for_timeout(200)
                assert page.locator('canvas').screenshot()!=moving_pixels
                if family=='Spinner':
                    assert page.evaluate('''()=>{const e=cuiGallery.exports;return Array.from({length:e.gallery_count()},(_,i)=>i).some(i=>{if(e.gallery_command(i,0)!==5)return false;const w=e.gallery_bitmap(i,0),h=e.gallery_bitmap(i,1),stride=e.gallery_bitmap(i,2);const pixels=new Uint8Array(e.memory.buffer,e.gallery_bitmap_pixels(i),stride*h);return w>0&&h>0&&pixels.some((v,j)=>j%4===3&&v>0);});}''')
                busy = page.locator('#accessible [aria-busy=true]')
                assert busy.count() == (1 if family == 'Skeleton' else 0)
                assert page.locator('#accessible [role=status]').count() == 0
                page.emulate_media(reduced_motion='reduce')
                page.wait_for_function('cuiGallery.renderedReducedMotion===true')
                page.wait_for_timeout(100)
                assert page.evaluate('cuiGallery.exports.gallery_next_wake()')==0
                assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
                idle=page.evaluate('cuiGallery.frame')
                screenshot=page.locator('canvas').screenshot()
                page.wait_for_timeout(180)
                assert page.evaluate('cuiGallery.frame')==idle
                assert page.locator('canvas').screenshot()==screenshot
                if family == 'Skeleton':
                    skeleton_preview(page)
                page.emulate_media(reduced_motion='no-preference')
            # Preserve existing Progress indeterminate/reduced-motion semantics.
            page.goto(f'http://127.0.0.1:{server.server_port}/#Progress')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Progress"')
            page.locator('#progress-mode').select_option('1')
            page.wait_for_function('document.querySelector("#accessible [role=progressbar][aria-busy=true]")!==null')
            before=page.evaluate('cuiGallery.frame')
            page.wait_for_function('(before)=>cuiGallery.frame>before+2',arg=before)
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('cuiGallery.renderedReducedMotion===true')
            page.wait_for_timeout(100)
            assert page.evaluate('cuiGallery.exports.gallery_redraw()')==0
            assert not errors,errors
            browser.close()
    finally: server.shutdown()
    print('Spinner/Skeleton scheduling, responsive Skeleton pixels and busy/loaded state, Progress semantics PASS')
if __name__=='__main__': main()
