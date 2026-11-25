# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual gallery tooltip appearance and Chart stroke and marker pixels."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

COMMANDS = """()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from(
 {length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),
 rect:[1,2,3,4].map(j=>e.gallery_command(i,j)),alpha:e.gallery_command(i,8),
 text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),
 e.gallery_command_length(i)))}));}"""

STROKE_PROFILE = """(scheme)=>{
  const e=cuiGallery.exports,canvas=document.querySelector('#preview');
  const image=Array.from({length:e.gallery_count()},(_,i)=>i)
    .find(i=>e.gallery_command(i,0)===5&&e.gallery_bitmap(i,1)>0);
  if(image===undefined)throw Error('Missing Chart bitmap');
  const [x,y,w,h]=[1,2,3,4].map(field=>e.gallery_command(image,field));
  const scale=canvas.width/canvas.clientWidth;
  const [left,right,top,bottom]=[x+w*.08,x+w*.92,y,y+h].map(v=>Math.round(v*scale));
  const pixels=canvas.getContext('2d').getImageData(left,top,right-left,bottom-top);
  const color=cuiGallery.themes.find(t=>t.id==='default-'+scheme).colors['chart.2'];
  const signal=parseInt(color.slice(5,7),16)-parseInt(color.slice(1,3),16);
  let minThickness=Infinity,minPeak=Infinity,breaks=0;
  for(let col=0;col<pixels.width;col++){
    let sum=0,peak=0;
    for(let row=0;row<pixels.height;row++){
      const at=(row*pixels.width+col)*4;
      // The default light and dark palettes use neutral grid and background colors.
      const coverage=Math.max(0,(pixels.data[at+2]-pixels.data[at])/signal);
      sum+=coverage;
      peak=Math.max(peak,coverage);
    }
    minThickness=Math.min(minThickness,sum/scale);
    minPeak=Math.min(minPeak,peak);
    if(peak<.75)breaks++;
  }
  return {minThickness,minPeak,breaks,scale};
}"""


DOT_PROFILE = """([scheme,centerY])=>{
  const e=cuiGallery.exports, canvas=document.querySelector('#preview');
  const image=Array.from({length:e.gallery_count()},(_,i)=>i)
    .find(i=>e.gallery_command(i,0)===5&&e.gallery_bitmap(i,1)>0);
  if(image===undefined)throw Error('Missing Chart bitmap');
  const [x,y,w,h]=[1,2,3,4].map(field=>e.gallery_command(image,field));
  const scale=canvas.width/canvas.clientWidth, cx=(x+w/5)*scale;
  const left=Math.round(cx)-Math.ceil(5*scale), top=Math.round(y*scale);
  const pixels=canvas.getContext('2d').getImageData(left,top,2*Math.ceil(5*scale)+1,
      Math.round((y+h)*scale)-top);
  const color=cuiGallery.themes.find(t=>t.id==='default-'+scheme).colors['chart.2'];
  const signal=parseInt(color.slice(5,7),16)-parseInt(color.slice(1,3),16);
  function ink(col,row){
    const at=(row*pixels.width+col)*4;
    return Math.max(0,(pixels.data[at+2]-pixels.data[at])/signal);
  }
  const col=Math.floor(cx)-left;
  const center=Array.from({length:pixels.height},(_,i)=>i).filter(i=>ink(col,i)>.75);
  if(!center.length)throw Error('Missing line at February');
  const cy=centerY ?? center.reduce((sum,i)=>sum+i+.5,0)/center.length;
  const sample=(dx,dy)=>ink(Math.floor(cx+dx*scale)-left,Math.floor(cy+dy*scale));
  return {cy,bottom:sample(0,2.8),outer:sample(0,5.6),corner:sample(3.3,3.3)};
}"""


def wait_for_chart_change(page, action):
    before = page.evaluate('cuiGallery.exports.gallery_chart_stats(4)')
    action()
    page.wait_for_function('before=>cuiGallery.exports.gallery_chart_stats(4)>before', arg=before)


def verify_dots(page, scheme):
    dots = page.get_by_role('checkbox', name='Data points')
    assert not dots.is_checked()
    baseline = page.evaluate(DOT_PROFILE, [scheme, None])
    assert baseline['bottom'] < .2, baseline
    wait_for_chart_change(page, dots.check)
    shown = page.evaluate(DOT_PROFILE, [scheme, baseline['cy']])
    assert shown['bottom'] > .75 and shown['outer'] < .2 and shown['corner'] < .4, shown
    wait_for_chart_change(page, dots.uncheck)
    assert page.evaluate(DOT_PROFILE, [scheme, baseline['cy']]) == baseline


def verify_stroke_pixels(browser, url):
    for width, scale, scheme in [(1280, 1, 'light'), (1280, 2, 'dark'), (390, 2, 'light')]:
        page = browser.new_page(viewport={'width': width, 'height': 900},
                                device_scale_factor=scale, color_scheme=scheme)
        try:
            page.goto(url + '/#Chart')
            page.wait_for_function('window.cuiGallery?.renderedComponent==="Chart"')
            curve = page.locator('#chart-curve')
            for value in ['0', '1', '2']:
                if curve.input_value() != value:
                    wait_for_chart_change(page, lambda: curve.select_option(value))
                profile = page.evaluate(STROKE_PROFILE, scheme)
                assert abs(profile['scale'] - scale) < .01, profile
                assert profile['minThickness'] >= 1.5 and profile['breaks'] == 0, (
                    width, scale, scheme, value, profile)
            wait_for_chart_change(page, lambda: curve.select_option('0'))
            verify_dots(page, scheme)
            kind = page.locator('#chart-kind')
            wait_for_chart_change(page, lambda: kind.select_option('1'))  # Area
            assert page.locator('#chart-dots-control').is_hidden()
            wait_for_chart_change(page, lambda: kind.select_option('4'))  # Radar
            assert page.locator('#chart-dots-control').is_visible()
        finally:
            page.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,default=Path('build/web'))
    p.add_argument('--chromium')
    args=p.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args): pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as pw:
            browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
            url=f'http://127.0.0.1:{server.server_port}'
            verify_stroke_pixels(browser, url)
            audit=Audit(browser,url)
            for width in [1280,390]:
                audit.page.set_viewport_size({'width':width,'height':900})
                audit.route('Chart')
                assert audit.page.locator('#chart-tooltip-appearance').is_visible()
                assert audit.page.locator('#chart-tooltip-appearance').input_value()==('0' if width==1280 else '2')
                for custom in [False,True]:
                    audit.configure('chart-custom-rows',custom)
                    sizes=[]
                    for appearance in [0,1,2]:
                        audit.page.locator('#chart-tooltip-appearance').select_option(str(appearance))
                        audit.page.wait_for_function('cuiGallery.exports.gallery_chart_stats(7)==='+str(appearance))
                        target=audit.target('Project activity chart')
                        point=audit.point(target,.08,.4)
                        audit.page.mouse.move(point['x'],point['y'])
                        wanted='Reporting period' if custom else 'Project activity chart'
                        # Automatic fallback uses the chart's supplied series name.
                        audit.page.wait_for_function('('+COMMANDS+')().some(c=>c.type===2&&c.alpha>0&&c.text==='+repr(wanted)+')')
                        commands=audit.page.evaluate(COMMANDS)
                        row=next(c for c in commands if c['type']==2 and c['alpha']>0 and c['text']==wanted)
                        sizes.append(row['rect'][3])
                        if appearance==2:
                            # Bare content begins at the plot origin, with a swatch slot
                            # only for automatic series rows; plain copied rows have none.
                            assert row['rect'][0] < target['f'][1]+40,(target,row)
                        else:
                            assert row['rect'][0] > target['f'][1]+40,(target,row)
                        assert audit.page.evaluate('cuiGallery.exports.gallery_error()')==0
                    assert sizes[1]==sizes[2] and sizes[0]!=sizes[1],sizes
            audit.page.locator('#component-guides').get_by_role('link',name='Opt-in Chart tooltip appearance').click()
            audit.page.locator('#docs-title').filter(has_text='Opt-in Chart tooltip appearance').wait_for(state='visible')
            assert audit.page.get_by_role('heading',name='Opt-in Chart tooltip appearance',exact=True).count()==1
            assert audit.page.locator('#doc-opt-in-chart-tooltip-appearance').count()==1
            assert not audit.errors,audit.errors
            browser.close()
        print('PASS gallery curve and dot pixels at desktop/mobile and 1×/2×, tooltip placement, rows and linked guide')
    finally:
        server.shutdown()

if __name__=='__main__': main()
