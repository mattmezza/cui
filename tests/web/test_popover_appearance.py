#!/usr/bin/env python3
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual composed Popover values and Canvas/WASM shadow pixels at DPR1/2."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--chromium')
    args = parser.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(ROOT/'scripts'))
    spec = importlib.util.spec_from_file_location('cui_build_web', ROOT/'scripts/build_web.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    sources = [s for s in module.SOURCES if not s.startswith('examples/') and s != 'web/gallery.c']
    exports = ['popover_scene','popover_command','popover_type','cui_canvas_shadow_values',
               'cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width',
               'cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound',
               'cui_canvas_shadow_pixels']
    wasm = args.output/'popover.wasm'
    assert subprocess.check_output(['zig','version'],text=True).strip() == '0.16.0'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2',
        '-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),
        '-I',str(ROOT/'third_party/clay'),*[str(ROOT/s) for s in sources],
        str(ROOT/'tests/web/popover_appearance_probe.c'),'-lm',
        *[f'-Wl,--export={e}' for e in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True, **({'executable_path':args.chromium} if args.chromium else {}))
        page=browser.new_page()
        result=page.evaluate('''async ([bytes,source])=>{
          const module=await WebAssembly.compile(new Uint8Array(bytes)),imports={};
          for(const i of WebAssembly.Module.imports(module))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();
          const m=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          const raster=m.cuiShadowWasmRasterizer(e),renderer=m.createCuiShadowRenderer(raster);
          let frames=0;
          for(const dark of [0,1])for(const translucent of [0,1])for(const scale of [1,2]){
            const count=e.popover_scene(dark,1,translucent),commands=[];
            for(let i=0;i<count;i++){
              const type=e.popover_type(i),v=Array.from(new Float32Array(e.memory.buffer,e.popover_command(i),16));
              commands.push(type===7?{type:'shadow',values:v}:type===3?{type:'clipPush',bounds:v.slice(0,4)}:type===4?{type:'clipPop'}:{type:'surface',values:v});
            }
            const shadows=commands.filter(c=>c.type==='shadow');
            if(shadows.length!==3||shadows[0].values[11]!==1||shadows[1].values[10]!==3||shadows[2].values[10]!==2)throw Error('actual source layers');
            if(shadows[0].values[12]!== (dark?230:30)||shadows[0].values[15]!==26)throw Error('foreground ring');
            const canvas=new OffscreenCanvas(128*scale,128*scale),ctx=canvas.getContext('2d');ctx.setTransform(scale,0,0,scale,0,0);
            const prepared=renderer.prepare(commands,ctx,128,128);ctx.fillStyle='white';ctx.fillRect(0,0,128,128);
            for(let i=0;i<commands.length;i++){
              const c=commands[i];if(c.type==='shadow')prepared.draw(i,ctx,{unclipped:true});
              else if(c.type==='surface'){const v=c.values;if(v[8]){ctx.fillStyle=`rgba(${v[5]},${v[6]},${v[7]},${v[8]/255})`;ctx.beginPath();ctx.roundRect(v[0],v[1],v[2],v[3],v[4]);ctx.fill();}}
            }
            // Independent per-layer raster plus Canvas compositor reference at a straight edge.
            const [x,y]=[64,82],control=new OffscreenCanvas(1,1),cc=control.getContext('2d');cc.fillStyle='white';cc.fillRect(0,0,1,1);
            for(const c of shadows){const r=raster(c.values,scale,0,0),ix=x*scale-Math.round(r.bounds[0]*scale),iy=y*scale-Math.round(r.bounds[1]*scale);
              if(ix>=0&&iy>=0&&ix<r.width&&iy<r.height){const q=(iy*r.width+ix)*4;cc.fillStyle=`rgba(${r.pixels[q]},${r.pixels[q+1]},${r.pixels[q+2]},${r.pixels[q+3]/255})`;cc.fillRect(0,0,1,1);}}
            const actual=ctx.getImageData(x*scale,y*scale,1,1).data,expected=cc.getImageData(0,0,1,1).data;
            if(Array.from(actual).some((v,k)=>Math.abs(v-expected[k])>2))throw Error(`outside pixel ${actual} vs ${expected}`);
            if(ctx.getImageData(64*scale,46*scale,1,1).data[0]===255)throw Error('actual surface absent');
            frames++;
          }
          for(const policy of [0,2]){const n=e.popover_scene(0,policy,0);for(let i=0;i<n;i++)if(e.popover_type(i)===7)throw Error('legacy/none shadow');}
          return {frames};
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()])
        assert result['frames']==8,result
        print('PASS actual Popover Canvas/WASM DPR1/2',result); browser.close()
if __name__=='__main__': main()
