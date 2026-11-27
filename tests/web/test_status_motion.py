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
    assert page.locator('#density').is_hidden()
    code = page.locator('#snippet').text_content()
    assert 'skeleton_example_draw' in code and 'SKELETON_EXAMPLE_PROFILE, loading, 16.0f' in code
    with page.expect_download() as source:
        page.get_by_role('link', name='Download the C recipe header').click()
    header = source.value.path().read_bytes()
    canonical = Path(__file__).resolve().parents[2] / 'examples/skeleton_recipes/scene.h'
    assert header == canonical.read_bytes()
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


def recipe_geometry(page, name, bar_height):
    return page.evaluate("""([name, barHeight]) => {
        const e = cuiGallery.exports;
        const text = new TextDecoder();
        const read = (address, length) => text.decode(new Uint8Array(e.memory.buffer, address, length));
        const region = Array.from({length: e.gallery_semantic_count()}, (_, i) => i).find(i =>
            read(e.gallery_semantic_name(i), e.gallery_semantic_length(i)) === name);
        const canvas = document.querySelector('canvas');
        const ctx = canvas.getContext('2d');
        const shapes = [], labels = [];
        for (let i = 0; i < e.gallery_count(); i++) {
            const f = n => e.gallery_command(i, n);
            if (f(0) === 2)
                labels.push(read(e.gallery_command_text(i), e.gallery_command_length(i)));
            if (f(0) !== 0 || (barHeight && f(4) !== barHeight) ||
                [5, 6, 7, 8].map(f).join() !== '241,245,249,255') continue;
            const [x, y, w, h] = [1, 2, 3, 4].map(f);
            const sample = ctx.getImageData(Math.floor(x + w/2), Math.floor(y + h/2), 1, 1);
            shapes.push({x, y, w, h, pixel: [...sample.data]});
        }
        const bounds = [1, 2, 3, 4].map(channel => e.gallery_semantic(region, channel));
        return {bounds, shapes, labels, width: canvas.clientWidth, height: canvas.clientHeight};
    }""", [name, bar_height])


def skeleton_recipes(page):
    page.emulate_media(reduced_motion='reduce')
    page.wait_for_function('cuiGallery.renderedReducedMotion===true')
    layout = page.locator('#skeleton-layout')
    rem_choice = page.locator('#skeleton-rem')
    loading = page.locator('#skeleton-state')
    page.locator('#skeleton-appearance').select_option('0')
    for choice, name, status, symbol, count in [
        ('1', 'Articles', 'Articles loaded', 'ARTICLES', 9),
        ('2', 'Workspace files', 'Files loaded', 'TABLE', 6),
    ]:
        page.emulate_media(reduced_motion='reduce')
        page.wait_for_function('cuiGallery.renderedReducedMotion===true')
        page.set_viewport_size({'width': 1200, 'height': 900})
        loading.select_option('1')
        layout.select_option(choice)
        region = page.locator(f'#accessible [role=region][aria-label="{name}"]')
        region.wait_for(state='attached')
        assert region.get_attribute('aria-busy') == 'true'
        assert region.locator('[tabindex],button,[role]').count() == 0
        # The same layout and scale must honor the available width at both sizes.
        for width, rem in [(1200, 12), (390, 20)]:
            page.set_viewport_size({'width': width, 'height': 900})
            rem_choice.select_option(str(rem))
            expected_height = rem * (2.5 if choice == '1' else .75)
            page.wait_for_function("""height => {
                const e = cuiGallery.exports;
                return Array.from({length: e.gallery_count()}, (_, i) => i).some(i =>
                    e.gallery_command(i, 0) === 0 && e.gallery_command(i, 4) === height &&
                    [5, 6, 7, 8].map(channel => e.gallery_command(i, channel)).join() === '241,245,249,255');
            }""", arg=expected_height, timeout=5000)
            bar_height = .75 * rem if choice == '2' else 0
            scene = recipe_geometry(page, name, bar_height)
            assert len(scene['shapes']) == count, scene
            assert max(s['h'] for s in scene['shapes']) == expected_height, scene
            x, y, w, h = scene['bounds']
            assert w <= 36 * rem, scene
            assert 0 <= x < x + w <= scene['width'], scene
            assert 0 <= y < y + h <= scene['height'], scene
            for shape in scene['shapes']:
                assert x <= shape['x'] < shape['x'] + shape['w'] <= x + w, scene
                assert y <= shape['y'] < shape['y'] + shape['h'] <= y + h, scene
                assert shape['pixel'] == [241, 245, 249, 255], shape
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
        code = page.locator('#snippet').text_content()
        assert f'SKELETON_EXAMPLE_{symbol}, loading, 20.0f' in code
        page.emulate_media(reduced_motion='no-preference')
        page.wait_for_function('cuiGallery.exports.gallery_next_wake() > 0')
        loading.select_option('0')
        region.locator(f'[role=status][aria-label="{status}"]').wait_for(state='attached')
        assert region.get_attribute('aria-busy') == 'false'
        assert page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
        ready = recipe_geometry(page, name, bar_height)
        bar_heights = {rem, .625 * rem} if choice == '1' else {.75 * rem}
        assert not any(shape['h'] in bar_heights for shape in ready['shapes']), ready
        assert ready['bounds'] == scene['bounds'], (ready, scene)
        if choice == '1':
            assert {'Design notes', 'Release recap', 'Field guide'} <= set(ready['labels']), ready
            items = region.locator('[role=list] [role=listitem]')
            assert [item.get_attribute('aria-label') for item in items.all()] == [
                'Design notes', 'Release recap', 'Field guide']
        else:
            assert {'File', 'Status'} <= set(scene['labels']), scene
            assert {'Notes', 'Images', 'Exports', 'Ready', 'Queued'} <= set(ready['labels']), ready
            table = region.locator('[role=table]')
            assert table.locator('[role=row]').count() == 4
            assert table.locator('[role=row] [role=columnheader]').count() == 2
            assert table.locator('[role=row] [role=cell]').count() == 6
        assert region.locator('[tabindex],button').count() == 0


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
                    skeleton_recipes(page)
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
    print('Spinner/Skeleton scheduling, three responsive Skeleton recipes and Progress semantics PASS')
if __name__=='__main__': main()
