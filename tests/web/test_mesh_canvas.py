#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Generic Canvas mesh masks, ordered blending, DPR and bounded preflight."""
import argparse
import math
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import threading
from pathlib import Path
from playwright.sync_api import sync_playwright
from test_touch_components import Audit

ROOT = Path(__file__).resolve().parents[2]


def reference(p, scale, clip, width=24, height=24):
    """Independent barycentric pixel-center oracle; Canvas supplies blend rounding."""
    result = []
    for k in range(0, p['vertexCount'], 3):
        v = [p['vertices'][j*4:j*4+4] for j in range(k, k+3)]
        v = [[(p['originX']+p['scale']*x)*scale,
              (p['originY']+p['scale']*y)*scale, s, t] for x, y, s, t in v]
        a, b, c = v
        determinant = (b[0]-a[0])*(c[1]-a[1])-(c[0]-a[0])*(b[1]-a[1])
        pixels = [0]*(width*scale*height*scale*4)
        if determinant == 0:
            result.append(pixels)
            continue
        if determinant < 0:
            b, c = c, b
            determinant = -determinant
        v = [a, b, c]
        sx = ((b[2]-a[2])*(c[1]-a[1])-(c[2]-a[2])*(b[1]-a[1]))/determinant
        sy = ((b[0]-a[0])*(c[2]-a[2])-(c[0]-a[0])*(b[2]-a[2]))/determinant
        tx = ((b[3]-a[3])*(c[1]-a[1])-(c[3]-a[3])*(b[1]-a[1]))/determinant
        ty = ((b[0]-a[0])*(c[3]-a[3])-(c[0]-a[0])*(b[3]-a[3]))/determinant
        alpha = p['color'][3]
        rgb = []
        for channel in p['color'][:3]:
            linear = min(1, max(0, channel/alpha if alpha else 0))
            rgb.append(math.floor(255*(12.92*linear if linear <= .0031308 else
                                       1.055*linear**(1/2.4)-.055)+.5))
        for y in range(height*scale):
            for x in range(width*scale):
                px, py = x+.5, y+.5
                if not (clip[0]*scale <= px < (clip[0]+clip[2])*scale and
                        clip[1]*scale <= py < (clip[1]+clip[3])*scale):
                    continue
                edges = [(v[(j+2)%3][0]-v[(j+1)%3][0])*(py-v[(j+1)%3][1])-
                         (v[(j+2)%3][1]-v[(j+1)%3][1])*(px-v[(j+1)%3][0])
                         for j in range(3)]
                if any(e < 0 or (e == 0 and not (v[(j+2)%3][1] < v[(j+1)%3][1] or
                       (v[(j+2)%3][1] == v[(j+1)%3][1] and v[(j+2)%3][0] > v[(j+1)%3][0])))
                       for j, e in enumerate(edges)):
                    continue
                coverage = 1
                if math.hypot(sx, sy) >= .001:
                    s = sum(edges[j]/determinant*v[j][2] for j in range(3))
                    t = sum(edges[j]/determinant*v[j][3] for j in range(3))
                    f = s*s-t
                    gradient = math.hypot(2*s*sx-tx, 2*s*sy-ty)
                    coverage = min(1, max(0, .5-f/gradient)) if gradient else (1 if f < 0 else 0 if f > 0 else .5)
                opacity = math.floor(alpha*coverage*255+.5)
                if opacity:
                    at = (y*width*scale+x)*4
                    pixels[at:at+4] = rgb+[opacity]
        result.append(pixels)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--chromium')
    parser.add_argument('--directory', type=Path, default=Path('build/web'))
    args = parser.parse_args()
    triangle = [2,2,0,1, 20,2,0,1, 2,20,0,1]
    vectors = []
    for vertices, color in [
        (triangle, [.25,.1,0,.5]),
        (triangle[:4]+triangle[8:]+triangle[4:8], [2,-1,.003,1]),
        ([2,2,-1,0, 20,2,1,0, 2,20,-1,1], [.3,.1,.2,.6]),
        ([0,0,-.5,0, 4,0,3.5,0, 0,4,-.5,0], [1,0,0,1]),
        ([0,0,0,0, 20,0,.01,0, 0,20,0,0], [0,1,0,1]),
        (triangle+[20,2,0,1,20,20,0,1,2,20,0,1], [.5,0,0,.5]),
        (triangle+triangle, [.25,.1,0,.5]),
        (triangle, [10,-4,3,0]),
        ([2,2,0,1]*3, [1,1,1,1]),
    ]:
        p = dict(vertexCount=len(vertices)//4, vertices=vertices, originX=0, originY=0, scale=1, color=color)
        for scale in (1, 2):
            for clip in ([0,0,24,24], [3.25,4.75,8.5,7]):
                vectors.append(dict(p=p, scale=scale, clip=clip, reference=reference(p, scale, clip)))
    vectors.append({**vectors[0], 'globalAlpha':.4})
    with sync_playwright() as playwright:
        browser = playwright.chromium.launch(headless=True, **({'executable_path':args.chromium} if args.chromium else {}))
        page = browser.new_page()
        result = page.evaluate('''async ([source,vectors])=>{
          const {createCuiMeshRenderer}=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          const renderer=createCuiMeshRenderer();let checked=0;
          const assert=(c,m)=>{if(!c)throw Error(m)};
          const read=c=>Array.from(c.getImageData(0,0,c.canvas.width,c.canvas.height).data);
          for(const v of vectors){
            const size=24*v.scale,ctx=new OffscreenCanvas(size,size).getContext('2d'),expected=new OffscreenCanvas(size,size).getContext('2d');
            ctx.setTransform(v.scale,0,0,v.scale,0,0);
            ctx.globalAlpha=expected.globalAlpha=v.globalAlpha??1;
            const commands=[{type:'clipPush',bounds:v.clip},{type:'mesh',mesh:structuredClone(v.p)},{type:'clipPop'}];
            const prepared=renderer.prepare(commands,ctx,24,24);
            commands[1].mesh.vertices.fill(999);commands[1].mesh.color.fill(0);
            for(const bytes of v.reference){const raster=new OffscreenCanvas(size,size),r=raster.getContext('2d');r.putImageData(new ImageData(new Uint8ClampedArray(bytes),size,size),0,0);expected.drawImage(raster,0,0);}
            prepared.draw(1,ctx,{unclipped:true});
            const actual=read(ctx),wanted=read(expected);
            assert(actual.every((x,i)=>x===wanted[i]),`exact pixels vector${checked}: ${actual.findIndex((x,i)=>x!==wanted[i])}`);
            checked++;
          }
          const ctx=new OffscreenCanvas(24,24).getContext('2d');ctx.fillStyle='purple';ctx.fillRect(0,0,24,24);const before=read(ctx),base=vectors[0].p;
          const rejects=(commands)=>{let bad=false;try{renderer.prepare(commands,ctx,24,24)}catch{bad=true}assert(bad,'expected rejection');assert(read(ctx).every((x,i)=>x===before[i]),'target changed on failure')};
          for(const matrix of [[1,1,0,1,0,0],[-1,0,0,-1,0,0],[1,0,0,2,0,0]]){ctx.setTransform(...matrix);rejects([{type:'mesh',mesh:base}]);}ctx.resetTransform();
          rejects([{type:'mesh',mesh:{...base,vertexCount:262146}}]);
          rejects([{type:'mesh',mesh:{...base,scale:Infinity}}]);
          rejects([{type:'clipPop'},{type:'mesh',mesh:base}]);
          rejects([{type:'clipPush',bounds:[0,0,1,1]},{type:'mesh',mesh:base}]);
          rejects(Array.from({length:65},()=>({type:'clipPush',bounds:[0,0,1,1]})).concat({type:'mesh',mesh:base}));
          rejects(Array.from({length:4097},()=>({type:'mesh',mesh:base})));
          let bounded=false;try{renderer.prepare([{type:'mesh',mesh:{...base,vertices:[0,0,0,1,4096,0,0,1,0,4096,0,1]}}],ctx,4096,4096)}catch{bounded=true}assert(bounded,'raster byte capacity');
          assert(read(ctx).every((x,i)=>x===before[i]),'capacity touched target');
          ctx.globalCompositeOperation='copy';rejects([{type:'mesh',mesh:base}]);ctx.globalCompositeOperation='source-over';
          const old=renderer.prepare([{type:'mesh',mesh:base}],ctx,24,24);renderer.prepare([],ctx,24,24);let expired=false;try{old.draw(0,ctx,{unclipped:true})}catch{expired=true}assert(expired,'expired frame');
          const moved=structuredClone(base);moved.originX=-10;moved.originY=-10;ctx.setTransform(2,0,0,2,20,20);
          const translated=renderer.prepare([{type:'mesh',mesh:moved}],ctx,24,24);ctx.clearRect(-10,-10,24,24);translated.draw(0,ctx,{unclipped:true});assert(ctx.getImageData(5,5,1,1).data[3]>0,'translated draw');
          return {checked};
        }''', [(ROOT/'web/modules/cui_mesh.js').read_text(), vectors])
        print(result)
        class Handler(SimpleHTTPRequestHandler):
            def log_message(self, *args):
                pass
        server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(args.directory.resolve())))
        threading.Thread(target=server.serve_forever, daemon=True).start()
        try:
            for dpr in (1, 2):
                context = browser.new_context(device_scale_factor=dpr)
                a = Audit.__new__(Audit)
                a.page = context.new_page()
                a.page.set_default_timeout(5000)
                a.page.set_viewport_size({'width':1280, 'height':900})
                a.page.goto(f'http://127.0.0.1:{server.server_port}/#Message')
                a.page.wait_for_function('window.cuiGallery?.renderedComponent === "Message"')
                actual = a.page.evaluate('''()=>{
                  const e=cuiGallery.exports,canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');
                  const baseline=ctx.getImageData(0,0,canvas.width,canvas.height);
                  e.gallery_renderer_fixture(2);cuiGallery.render();
                  const i=Array.from({length:e.gallery_count()},(_,i)=>i).find(i=>e.gallery_command(i,0)===6);
                  if(i===undefined)throw Error('Core mesh not emitted');
                  const alpha=e.gallery_command(i,78),red=e.gallery_command(i,75),dpr=ctx.getTransform().a;
                  if(alpha!==.25||red!==.25)throw Error('Core ancestor opacity missing');
                  const x=Math.floor((e.gallery_command(i,72)+2)*dpr),y=Math.floor((e.gallery_command(i,73)+2)*dpr);
                  const reference=new OffscreenCanvas(canvas.width,canvas.height).getContext('2d');reference.putImageData(baseline,0,0);
                  reference.fillStyle='rgba(255,0,0,'+(64/255)+')';reference.fillRect(x,y,1,1);
                  const actual=Array.from(ctx.getImageData(x,y,1,1).data),expected=Array.from(reference.getImageData(x,y,1,1).data);
                  if(actual.some((v,i)=>v!==expected[i]))throw Error(`Core opacity pixels ${actual} vs ${expected}`);
                  e.gallery_renderer_fixture(0);cuiGallery.render();
                  if(Array.from({length:e.gallery_count()},(_,i)=>e.gallery_command(i,0)).includes(6))throw Error('fixture teardown');
                  return {dpr,alpha,actual};
                }''')
                print(actual)
                context.close()
        finally:
            server.shutdown()
        browser.close()


if __name__ == '__main__':
    main()
