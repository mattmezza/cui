#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual hosted controlled Slider: full-pair acceptance, pixels, input and idle."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
from test_questionnaire_progress_gallery import compile_usage
from test_control_surface_shadow_gallery import SHADOWS, BASELINE, PIXEL_DIFF


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--compile-usage', action='store_true')
    args = parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *_): pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    cases = []
    try:
        with sync_playwright() as pw:
            browser = pw.chromium.launch(headless=True)
            for width, dpr in ((1280, 1), (390, 2)):
                context = browser.new_context(viewport={'width': width, 'height': 1000},
                                              device_scale_factor=dpr, has_touch=True,
                                              reduced_motion='reduce')
                page = context.new_page()
                a = Audit.__new__(Audit)
                a.page, a.errors, a.active_touch = page, [], False
                a.session = context.new_cdp_session(page)
                page.on('pageerror', lambda error: a.errors.append(str(error)))
                page.goto(f'http://127.0.0.1:{server.server_port}/#Slider')
                page.wait_for_function('window.cuiGallery?.renderedComponent === "Slider"')
                def stat(field):
                    return page.evaluate('(f)=>cuiGallery.exports.gallery_slider_stats(f)', field)
                def pair(): return [stat(0), stat(1)]
                def config(key, value): a.configure('slider-' + key, value)
                def end(): return a.target('Selection end')
                def image(): return page.locator('canvas').screenshot()
                def track_pixels():
                    # Exclude changing owner/request text: prove actual slider paint moves.
                    return page.evaluate("""()=>{const e=cuiGallery.exports,c=document.querySelector('canvas'),
                      ctx=c.getContext('2d'),scale=ctx.getTransform().a,
                      r=[0,1,2,3].map(f=>e.gallery_slider_part_bounds(1,f)),
                      x=Math.max(0,Math.floor((r[0]-16)*scale)),y=Math.max(0,Math.floor((r[1]-16)*scale)),
                      right=Math.min(c.width,Math.ceil((r[0]+r[2]+16)*scale)),
                      bottom=Math.min(c.height,Math.ceil((r[1]+r[3]+16)*scale));
                      return Array.from(ctx.getImageData(x,y,right-x,bottom-y).data);}
                    """)
                def clean():
                    assert page.evaluate('cuiGallery.exports.gallery_error()') == 0
                    assert stat(9) == 0
                    assert not a.errors, a.errors
                def idle():
                    page.mouse.move(1, 1)
                    page.wait_for_function('cuiGallery.exports.gallery_next_wake() === 0')
                    before = page.evaluate('cuiGallery.frame')
                    shot = image()
                    page.wait_for_timeout(150)
                    assert page.evaluate('cuiGallery.frame') == before
                    assert image() == shot
                assert page.locator('#slider-kind').input_value() == '0'
                assert pair() == [0, 40]
                assert end()['f'][3] > 0
                for axis in (0, 1):
                    config('axis', axis)
                    for kind in (0, 1):
                        config('kind', kind)
                        before = pair()
                        config('accept', 0)
                        target = end(); point = a.point(target)
                        page.mouse.click(point['x'], point['y'])
                        page.keyboard.press('ArrowLeft')
                        page.wait_for_timeout(80)
                        assert pair() == before and stat(8) == 0
                        assert stat(5) < before[1] and stat(3) == before[1], {'axis': axis, 'kind': kind, 'before': before, 'current': stat(3), 'requested': stat(5), 'count': stat(7), 'phase': stat(6)}
                        config('accept', 1)
                        target = end(); point = a.point(target)
                        page.mouse.click(point['x'], point['y'])
                        baseline = image(); painted_before_keys = track_pixels(); accepted_before_keys = stat(8)
                        page.keyboard.press('ArrowLeft')
                        page.keyboard.press('ArrowLeft')
                        page.wait_for_timeout(80)
                        assert pair()[0] == before[0] and pair()[1] == before[1] - 2
                        assert stat(8) == accepted_before_keys + 2 and image() != baseline, {'accepted': stat(8), 'pair': pair(), 'before': before, 'axis': axis, 'kind': kind, 'pixels_changed': image() != baseline}
                        assert track_pixels() != painted_before_keys, 'owner text alone is not painted thumb evidence'
                        if kind:
                            start = a.target('Selection start')
                            assert start['id'] != end()['id']
                            assert start['f'][1:3] != end()['f'][1:3]
                        if args.compile_usage:
                            compile_usage(page.locator('#snippet').inner_text())
                        # Named inset decorates the actual new track with source-derived radius.
                        page.mouse.move(1, 1); page.wait_for_timeout(50)
                        page.evaluate(BASELINE)
                        a.configure('control-shadows', True)
                        shadows = page.evaluate(SHADOWS)
                        assert len(shadows) == 1 and shadows[0]['type'] == 8
                        track = page.evaluate('()=>[0,1,2,3].map(f=>cuiGallery.exports.gallery_slider_part_bounds(1,f))')
                        assert all(abs(x-y) < .001 for x,y in zip(shadows[0]['v'][:4], track))
                        assert page.evaluate(PIXEL_DIFF, shadows) >= 8
                        assert page.evaluate('cuiGallery.exports.gallery_control_stats(14)') == 0
                        if args.compile_usage:
                            compile_usage(page.locator('#snippet').inner_text())
                        a.configure('control-shadows', False)
                        assert page.evaluate(SHADOWS) == []
                        # Actual touch contact changes owner and painted thumb on both axes.
                        old = pair(); baseline = image(); painted_before_touch = track_pixels()
                        a.drag(end(), 18 if not axis else 0, -18 if axis else 0)
                        assert pair() != old and image() != baseline
                        assert track_pixels() != painted_before_touch
                        if kind: assert pair()[0] == old[0]
                        # Disabled input produces no callback, accepted pair remains stable.
                        a.configure('disabled', True)
                        old = pair(); count = stat(7)
                        a.tap(end())
                        assert pair() == old and stat(7) == count
                        a.configure('disabled', False)
                        clean(); idle()
                        cases.append({'width': width, 'dpr': dpr, 'axis': axis, 'kind': kind})
                # Change current kind/domain/axis while actual pointer capture is held.
                # Base receives old retained gesture and emits captured CANCEL before new input.
                for key, value, old_kind, old_min in (('kind', 0, 1, 0), ('scale', 1, 0, 0),
                                                       ('axis', 0, 0, 1)):
                    target = end(); point = a.point(target)
                    page.mouse.move(point['x'], point['y']); page.mouse.down()
                    page.wait_for_timeout(50)
                    assert stat(17) == 1
                    config(key, value)
                    assert stat(13) == 1 and stat(15) == old_kind and stat(16) == old_min
                    assert stat(17) == 0
                    old = pair()
                    page.mouse.move(point['x'] + 12, point['y'] + 12); page.mouse.up()
                    page.wait_for_timeout(70)
                    assert pair() == old, 'cancelled capture must not leave an orphan drag'
                    clean()
                # Reverse/log semantics, bounds and explicit positive-domain config reset.
                config('scale', 1)
                forward_fill = page.evaluate('()=>[0,1,2,3].map(f=>cuiGallery.exports.gallery_slider_part_bounds(2,f))')
                config('reverse', 1)
                reverse_fill = page.evaluate('()=>[0,1,2,3].map(f=>cuiGallery.exports.gallery_slider_part_bounds(2,f))')
                assert reverse_fill != forward_fill, 'reverse must change actual painted fill geometry'
                a.tap(end()); page.keyboard.press('End'); page.wait_for_timeout(80)
                assert pair()[1] == 100
                page.keyboard.press('Home'); page.wait_for_timeout(80)
                assert pair()[1] == 1  # Single positive-domain minimum.
                clean()
                config('kind', 0); page.mouse.move(1, 1)
                # Actual host timer wakes a hovered spring and stops without manual render calls.
                page.emulate_media(reduced_motion='no-preference')
                page.wait_for_timeout(80)
                target = end(); point = a.point(target)
                before = page.evaluate('cuiGallery.frame')
                baseline = image()
                page.mouse.move(point['x'], point['y'])
                page.wait_for_function('cuiGallery.exports.gallery_next_wake() > 0')
                page.wait_for_function('(f)=>cuiGallery.frame > f+2', arg=before)
                assert image() != baseline
                page.wait_for_function('cuiGallery.exports.gallery_next_wake() === 0')
                idle()
                page.emulate_media(reduced_motion='reduce')
                page.wait_for_timeout(80)
                point = a.point(end()); page.mouse.move(point['x'], point['y'])
                page.wait_for_timeout(80)
                assert stat(10) == 0
                idle(); clean()
                # Real current geometry responds to viewport resize, not cached fixture rects.
                config('axis', 0)
                wide = page.evaluate('cuiGallery.exports.gallery_slider_part_bounds(1,2)')
                page.set_viewport_size({'width': 760 if width == 1280 else 430, 'height': 1000})
                page.wait_for_timeout(150)
                assert page.evaluate('cuiGallery.exports.gallery_slider_part_bounds(1,2)') != wide
                clean()
                context.close()
            browser.close()
    finally:
        server.shutdown()
    if args.report:
        args.report.write_text(json.dumps({'status': 'passed', 'cases': cases}, indent=2) + '\n')
    print('PASS typed Slider actual owner acceptance, full pairs, pixels, mouse/touch/keys and passive idle')


if __name__ == '__main__':
    main()
