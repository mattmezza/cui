#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual Gallery shadows, stationary cache reuse, and resize preflight rollback."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright


SHADOWS = """()=>{const e=cuiGallery.exports;return Array.from(
 {length:e.gallery_count()},(_,i)=>i).filter(i=>e.gallery_command(i,0)===7)
 .map(i=>Array.from({length:16},(_,f)=>e.gallery_shadow_value(i,f)));}"""

PIXELS = """()=>{
 const e=cuiGallery.exports,c=document.querySelector('canvas'),ctx=c.getContext('2d');
 const scale=ctx.getTransform().a,shadows=(SHADOWS)(),base=window.__shadowBaseline;
 if(base.width!==c.width||base.height!==c.height)throw Error('baseline resized');
 const caster=shadows[0],points=[.25,2.25,4.25].map(d=>[
   Math.floor((caster[0]+caster[2]/2)*scale),Math.floor((caster[1]+caster[3]+d)*scale)]);
 let changed=0;
 const results=[];
 for(const [px,py] of points){
   if(px<0||py<0||px>=c.width||py>=c.height)throw Error('probe outside target');
   const i=(py*c.width+px)*4;
   let expected=Array.from(base.data.slice(i,i+4));
   for(const v of shadows){
     new Float32Array(e.memory.buffer,e.cui_canvas_shadow_values(),16).set(v);
     if(!e.cui_canvas_shadow_prepare(scale,0,0))throw Error('reference raster failed');
     const width=e.cui_canvas_shadow_width(),height=e.cui_canvas_shadow_height();
     const x=px-Math.round(e.cui_canvas_shadow_bound(0)*scale);
     const y=py-Math.round(e.cui_canvas_shadow_bound(1)*scale);
     const pixels=new Uint8Array(e.memory.buffer,e.cui_canvas_shadow_pixels(),width*height*4);
     const alpha=x>=0&&y>=0&&x<width&&y<height?pixels[(y*width+x)*4+3]/255:0;
     const oldAlpha=expected[3]/255,outAlpha=alpha+oldAlpha*(1-alpha);
     expected=[0,1,2].map(k=>outAlpha?(v[12+k]*alpha+expected[k]*oldAlpha*(1-alpha))/outAlpha:0)
       .concat(outAlpha*255);
   }
   const actual=Array.from(ctx.getImageData(px,py,1,1).data);
   if(!expected.every(Number.isFinite)||actual.some((v,k)=>Math.abs(v-expected[k])>2))
     throw Error(`shadow pixel ${px},${py}: ${actual} vs ${expected}`);
   if(actual.slice(0,3).some((v,k)=>Math.abs(v-base.data[i+k])>=3))changed++;
   results.push({point:[px,py],actual,expected});
 }
 if(changed<2)throw Error('shadow probes did not distinguish painted tail from baseline');
 return results;
}""".replace("SHADOWS", SHADOWS)

REJECT_RESIZE = """()=>{
 const c=document.querySelector('canvas'),ctx=c.getContext('2d');
 const before=ctx.getImageData(0,0,c.width,c.height),oldWidth=c.width,oldHeight=c.height;
 const previousCss=c.style.width,original=ctx.getTransform;
 const own=Object.prototype.hasOwnProperty.call(ctx,'getTransform');
 const frame=cuiGallery.frame;
 c.style.width=`${c.clientWidth-23}px`;
 const wanted=Math.round(c.clientWidth*Math.min(devicePixelRatio,3));
 if(wanted===oldWidth)throw Error('fixture did not request a backing resize');
 Object.defineProperty(ctx,'getTransform',{configurable:true,value:function(){
   const t=original.call(this);t.d=2*t.a;return t;
 }});
 let error;
 try{cuiGallery.render();}catch(e){error=String(e);}
 finally{if(own)Object.defineProperty(ctx,'getTransform',{configurable:true,value:original});
   else delete ctx.getTransform;}
 if(!error?.includes('nonuniform shadow transform'))throw Error(`preflight did not reject: ${error}`);
 if(c.width!==oldWidth||c.height!==oldHeight||cuiGallery.frame!==frame)
   throw Error('failed preflight published backing dimensions or rendered frame');
 const after=ctx.getImageData(0,0,c.width,c.height);
 if(after.data.length!==before.data.length||after.data.some((v,i)=>v!==before.data[i]))
   throw Error('failed preflight cleared or partially painted existing pixels');
 cuiGallery.render();
 if(c.width!==wanted||c.height!==oldHeight||cuiGallery.frame!==frame+1)
   throw Error('valid resize recovery did not publish the new backing target');
 if((SHADOWS)().length!==3)throw Error('valid resize recovery lost tooltip shadows');
 const recovered=ctx.getImageData(0,0,c.width,c.height);
 if(!recovered.data.some((v,i)=>i%4===3&&v>0))throw Error('recovery target remained empty');
 c.style.width=previousCss;cuiGallery.render();
 return {oldWidth,oldHeight,wanted,error,unchangedBytes:before.data.length};
}""".replace("SHADOWS", SHADOWS)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, default=Path("build/web"))
    parser.add_argument("--chromium")
    args = parser.parse_args()

    class Handler(SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

    server = ThreadingHTTPServer(
        ("127.0.0.1", 0), partial(Handler, directory=str(args.directory.resolve()))
    )
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        with sync_playwright() as pw:
            options = {"executable_path": args.chromium} if args.chromium else {}
            browser = pw.chromium.launch(headless=True, **options)
            cases = []
            for dpr in [1, 2]:
                for width in [1280, 390]:
                    page = browser.new_page(
                        viewport={"width": width, "height": 900}, device_scale_factor=dpr
                    )
                    page.set_default_timeout(10000)
                    errors = []
                    page.on("pageerror", lambda error: errors.append(str(error)))
                    page.emulate_media(reduced_motion="reduce")
                    page.goto(f"http://127.0.0.1:{server.server_port}/#Chart")
                    page.wait_for_function('window.cuiGallery?.renderedComponent==="Chart"')
                    page.locator("canvas").scroll_into_view_if_needed()
                    page.locator("#chart-tooltip-appearance").select_option("1")
                    page.wait_for_function("cuiGallery.exports.gallery_chart_stats(7)===1")
                    for custom in [False, True]:
                        page.mouse.move(1, 1)
                        page.wait_for_function(f"({SHADOWS})().length===0")
                        page.evaluate("""custom=>{const control=document.getElementById('chart-custom-rows');
                          control.checked=custom;control.dispatchEvent(new Event('change',{bubbles:true}));}""", custom)
                        page.wait_for_function(
                            "custom=>cuiGallery.exports.gallery_chart_stats(6)===Number(custom)",
                            arg=custom,
                        )
                        page.evaluate("""()=>{cuiGallery.render();const c=document.querySelector('canvas');
                          window.__shadowBaseline=c.getContext('2d').getImageData(0,0,c.width,c.height);}""")
                        point = page.evaluate("""()=>{const e=cuiGallery.exports,d=new TextDecoder();
                          for(let i=0;i<e.gallery_semantic_count();i++){
                            const name=d.decode(new Uint8Array(e.memory.buffer,e.gallery_semantic_name(i),e.gallery_semantic_length(i)));
                            if(name==='Project activity chart'){const b=document.querySelector('canvas').getBoundingClientRect();
                              return {x:b.x+e.gallery_semantic(i,1)+e.gallery_semantic(i,3)*.08,
                                y:b.y+e.gallery_semantic(i,2)+e.gallery_semantic(i,4)*.4};}}
                          throw Error('missing chart semantic target');}""")
                        page.mouse.move(point["x"], point["y"])
                        page.wait_for_function(f"({SHADOWS})().length===3&&({SHADOWS})().every(v=>v[15]===26)")
                        wanted = "Reporting period" if custom else "Project activity chart"
                        page.wait_for_function("""wanted=>{const e=cuiGallery.exports,d=new TextDecoder();
                          for(let i=0;i<e.gallery_count();i++)if(e.gallery_command(i,0)===2&&e.gallery_command(i,8)>0&&
                            d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))===wanted)return true;
                          return false;}""", arg=wanted)
                        shadows = page.evaluate(SHADOWS)
                        assert [v[8:12] for v in shadows] == [
                            [0, 0, 0, 1], [0, 4, 3, -1], [0, 2, 2, -2]
                        ], shadows
                        assert all(v[:8] == shadows[0][:8] for v in shadows), shadows
                        assert shadows[1][12:16] == shadows[2][12:16] == [0, 0, 0, 26]
                        pixels = page.evaluate(PIXELS)
                        reuse = page.evaluate("""()=>{const before=cuiGallery.shadowStats();
                          for(let i=0;i<3;i++)cuiGallery.render();const after=cuiGallery.shadowStats();
                          return {hits:after.hits-before.hits,misses:after.misses-before.misses};}""")
                        assert reuse["hits"] >= 9 and reuse["misses"] == 0, reuse
                        rejection = page.evaluate(REJECT_RESIZE)
                        assert page.evaluate("cuiGallery.exports.gallery_error()") == 0
                        cases.append({"dpr": dpr, "width": width, "custom": custom,
                                      "pixels": pixels, "reuse": reuse, "rejection": rejection})
                    assert not errors, errors
                    page.close()
            browser.close()
        print("PASS actual Gallery automatic/copied boxed shadows, DPR1/2 desktop/390px, "
              "source layer values/pixels, stationary cache, preclear resize rejection and recovery:",
              [{k: case[k] for k in ["dpr", "width", "custom", "reuse", "rejection"]} for case in cases])
    finally:
        server.shutdown()


if __name__ == "__main__":
    main()
