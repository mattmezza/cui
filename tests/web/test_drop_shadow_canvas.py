#!/usr/bin/env python3
"""Actual Canvas+pure C shadow pixels, DPR and pre-clear rejection."""
import argparse
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--chromium');parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True);wasm=args.output/'shadow.wasm'
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['cui_canvas_shadow_values','cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width','cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound','cui_canvas_shadow_pixels']
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/p) for p in ['src/core/cui.c','src/effects/cui_effects.c','web/modules/cui_shadow_bridge.c']],'-lm',*[f'-Wl,--export={e}' for e in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}));page=browser.new_page()
        result=page.evaluate('''async ([bytes,source])=>{
          const module=await WebAssembly.compile(new Uint8Array(bytes)),imports={};for(const i of WebAssembly.Module.imports(module))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();const m=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          const raster=m.cuiShadowWasmRasterizer(e),renderer=m.createCuiShadowRenderer(raster),results=[];
          const base=[20,20,24,16,4,4,4,4,0,4,3,-1,0,0,0,128];
          function same(a,b){if(a.length!==b.length||!a.concat(b).every(Number.isFinite)||a.some((x,i)=>Math.abs(x-b[i])>1))throw Error(`pixels ${a} vs ${b}`);}
          for(const scale of [1,2]){
            const canvas=new OffscreenCanvas(Math.round(64.25*scale),64*scale),ctx=canvas.getContext('2d');ctx.setTransform(scale,0,0,scale,0,0);
            const commands=[{type:'shadow',values:base.slice()}];let prepared=renderer.prepare(commands,ctx,64.25,64);
            commands[0].values.fill(999);commands[0].type='destroyed'; // preflight copies all values/type
            ctx.fillStyle='white';ctx.fillRect(0,0,64.25,64);if(!prepared.draw(0,ctx,{unclipped:true}))throw Error('borrowed command');
            const expected=raster(base,scale,0,0);
            const points=[[18,28],[30,30],[30,40],[10,10]];
            for(const [x,y] of points){const px=x*scale,py=y*scale,ix=Math.floor(px+.5-expected.bounds[0]*scale),iy=Math.floor(py+.5-expected.bounds[1]*scale);const alpha=ix>=0&&iy>=0&&ix<expected.width&&iy<expected.height?expected.pixels[(iy*expected.width+ix)*4+3]/255:0;same(Array.from(ctx.getImageData(px,py,1,1).data),[255*(1-alpha),255*(1-alpha),255*(1-alpha),255]);}
            const before=Array.from(ctx.getImageData(0,0,canvas.width,canvas.height).data);
            for(const bad of [NaN,Infinity,1e-45,-1,257]){const v=base.slice();v[10]=bad;let rejected=false;try{renderer.prepare([{type:'shadow',values:v}],ctx,64.25,64);}catch{rejected=true;}if(!rejected)throw Error('invalid accepted');same(before,Array.from(ctx.getImageData(0,0,canvas.width,canvas.height).data));}
            for(const bounds of [[NaN,0,4,4],[0,0,-1,4],[0,0,4,Infinity]]){let rejected=false;try{renderer.prepare([{type:'clipPush',bounds},{type:'shadow',values:base.slice()},{type:'clipPop'}],ctx,64.25,64);}catch{rejected=true;}if(!rejected)throw Error('invalid mask accepted');same(before,Array.from(ctx.getImageData(0,0,canvas.width,canvas.height).data));}
            ctx.setTransform(scale,0,0,scale*2,0,0);let rejected=false;try{renderer.prepare([{type:'shadow',values:base}],ctx,64.25,64);}catch{rejected=true;}if(!rejected)throw Error('anisotropic accepted');
            if(renderer.prepare([{type:'other'}],ctx,64.25,64).draw(0,ctx))throw Error('legacy changed');
            ctx.setTransform(scale,0,0,scale,0,0);
            const tail=[{type:'clipPush',bounds:[10,20,7,16]},{type:'shadow',values:base.slice()},{type:'clipPop'}];prepared=renderer.prepare(tail,ctx,64.25,64);ctx.fillStyle='white';ctx.fillRect(0,0,64.25,64);prepared.draw(1,ctx,{unclipped:true});same(Array.from(ctx.getImageData(16*scale,28*scale,1,1).data),[255,255,255,255]);
            const fractional=[{type:'clipPush',bounds:[18.25,22.25,3,5]},{type:'shadow',values:base.slice()},{type:'clipPop'}];
            prepared=renderer.prepare(fractional,ctx,64.25,64);ctx.fillStyle='white';ctx.fillRect(0,0,64.25,64);prepared.draw(1,ctx,{unclipped:true});
            const original=raster(base,scale,0,0),px=Math.floor(18.25*scale),py=Math.floor(23.25*scale);
            const ix=px-Math.round(original.bounds[0]*scale),iy=py-Math.round(original.bounds[1]*scale),a=original.pixels[(iy*original.width+ix)*4+3]/255;
            same(Array.from(ctx.getImageData(px,py,1,1).data),[255*(1-a),255*(1-a),255*(1-a),255]);
            same(Array.from(ctx.getImageData(Math.ceil(21.25*scale),py,1,1).data),[255,255,255,255]);
            const collapsed=[20.25,20.25,1,1,0,0,0,0,0,0,0,-.6,0,0,0,255];
            prepared=renderer.prepare([{type:'shadow',values:collapsed}],ctx,64.25,64);
            ctx.fillStyle='white';ctx.fillRect(0,0,64.25,64);prepared.draw(0,ctx,{unclipped:true});
            const covered=scale===1?20:41;
            same(Array.from(ctx.getImageData(covered,covered,1,1).data),[0,0,0,255]);
            same(Array.from(ctx.getImageData(covered+1,covered,1,1).data),[255,255,255,255]);
            results.push({scale,actualTransform:ctx.getTransform().a,backingWidth:canvas.width,listWidth:64.25});
          }
          // Integer translated caster and opacity reuse coverage; fractional phase does not.
          const c=new OffscreenCanvas(128,128).getContext('2d'),r=m.createCuiShadowRenderer(raster);r.prepare([{type:'shadow',values:base.slice()}],c,128,128);const moved=base.slice();moved[0]++;moved[15]=64;r.prepare([{type:'shadow',values:moved}],c,128,128);if(r.stats().misses!==1||r.stats().hits!==1)throw Error('cache movement/opacity');moved[0]+=.25;r.prepare([{type:'shadow',values:moved}],c,128,128);if(r.stats().misses!==2)throw Error('phase cache');
          const old=r.prepare([{type:'shadow',values:base.slice()}],c,128,128);r.prepare([{type:'other'}],c,128,128);let expired=false;try{old.draw(0,c,{unclipped:true});}catch{expired=true;}if(!expired)throw Error('old prepared lifetime');
          const high=base.slice();high[0]=16384;high[2]=Math.fround(1.0001);high[8]=high[9]=0;high[11]=0;
          c.setTransform(1,0,0,1,-16384,0);const highRenderer=m.createCuiShadowRenderer(raster),highCommands=[{type:'clipPush',bounds:[16384,0,1,64]},{type:'shadow',values:high},{type:'clipPop'}];
          const highPrepared=highRenderer.prepare(highCommands,c,16392,128);c.fillStyle='white';c.fillRect(16384,0,128,128);highPrepared.draw(1,c,{unclipped:true});
          const highRaster=raster(high,1,-16384,0),alpha=highRaster.pixels[((25-Math.round(highRaster.bounds[1]))*highRaster.width-Math.round(highRaster.bounds[0]-16384))*4+3]/255;
          same(Array.from(c.getImageData(0,25,1,1).data),[255*(1-alpha),255*(1-alpha),255*(1-alpha),255]);
          // Literal source far edge: materialized f32 origin+size covers x33,
          // whereas the former algebraic rewrite stopped at x32.
          const far=[Math.fround(4000.1001),20.25,32,4,0,0,0,0,1.5,0,0,Math.fround(-.6),0,0,0,255];
          c.setTransform(1,0,0,1,-4000,0);
          const farRenderer=m.createCuiShadowRenderer(raster),farPrepared=farRenderer.prepare([{type:'shadow',values:far}],c,4040,128);
          c.fillStyle='white';c.fillRect(4000,0,128,128);farPrepared.draw(0,c,{unclipped:true});
          same(Array.from(c.getImageData(33,21,1,1).data),[0,0,0,255]);
          same(Array.from(c.getImageData(34,21,1,1).data),[255,255,255,255]);
          return {results,cache:r.stats()};
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()])
        assert [r['scale'] for r in result['results']]==[1,2],result
        print('PASS actual Canvas DPR1/2 shadow pixels, rounded backing vs actual uniform transform, copied values, rejection before clear, tail culling and cache:',result);browser.close()
if __name__=='__main__':main()
