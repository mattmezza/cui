#!/usr/bin/env python3
"""Pinned named list values and actual Canvas/WASM composition at DPR1/2."""
import argparse
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--chromium');p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    wasm=a.output/'presets.wasm'
    exports=['shadow_preset_layer','cui_canvas_shadow_values','cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width','cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound','cui_canvas_shadow_pixels']
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/x) for x in ['src/core/cui.c','src/effects/cui_effects.c','web/modules/cui_shadow_bridge.c','tests/web/shadow_presets_probe.c']],'-lm',*[f'-Wl,--export={x}' for x in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        b=p.chromium.launch(headless=True,**({'executable_path':a.chromium} if a.chromium else {}));page=b.new_page()
        result=page.evaluate('''async ([bytes,source])=>{
          const module=await WebAssembly.compile(new Uint8Array(bytes)),imports={};for(const i of WebAssembly.Module.imports(module))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();const m=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          const expected=[[],[[1,0,0,13]],[[1,2,0,13]],[[1,3,0,26],[1,2,-1,26]],[[4,6,-1,26],[2,4,-2,26]],[[10,15,-3,26],[4,6,-4,26]],[[20,25,-5,26],[8,10,-6,26]],[[25,50,-12,64]]];
          let frames=0;
          for(const scale of [1,2])for(let preset=0;preset<8;preset++){
            const c=new OffscreenCanvas(128*scale,128*scale),ctx=c.getContext('2d');ctx.setTransform(scale,0,0,scale,0,0);
            const commands=[];const count=e.shadow_preset_layer(preset,0);if(count!==expected[preset].length)throw Error('source count');
            for(let i=0;i<count;i++){
              e.shadow_preset_layer(preset,i);const values=Array.from(new Float32Array(e.memory.buffer,e.cui_canvas_shadow_values(),16));
              const tuple=[values[9],values[10],values[11],values[15]];if(tuple.some((x,k)=>x!==expected[preset][i][k])||values[8]!==0||values.slice(12,15).some(x=>x!==0))throw Error('source ordered values');
              commands.push({type:'shadow',values});
            }
            const r=m.createCuiShadowRenderer(m.cuiShadowWasmRasterizer(e)),prepared=r.prepare(commands,ctx,128,128);
            ctx.fillStyle='white';ctx.fillRect(0,0,128,128);for(let i=0;i<count;i++)prepared.draw(i,ctx,{unclipped:true});
            const center=ctx.getImageData(52*scale,52*scale,1,1).data;
            if(preset===0&&center[0]!==255)throw Error('none paints');
            if(preset===1&&Math.abs(center[0]-242)>1)throw Error(`2xs alpha13 scale${scale}: ${Array.from(center)}`);
            if(preset===3){
              const control=new OffscreenCanvas(1,1).getContext('2d');control.fillStyle='white';control.fillRect(0,0,1,1);
              const solid=new OffscreenCanvas(1,1);solid.getContext('2d').fillRect(0,0,1,1);
              control.globalAlpha=26/255;control.drawImage(solid,0,0);control.drawImage(solid,0,0);
              const expectedCenter=control.getImageData(0,0,1,1).data[0];
              if(Math.abs(center[0]-expectedCenter)>1)throw Error(`SM two-layer scale${scale}: ${center[0]} vs Canvas compositor ${expectedCenter}`);
            }
            if(preset>0&&center[0]>=255)throw Error('preset absent');
            if(preset===1){
              commands[0].values[15]=6;const half=r.prepare(commands,ctx,128,128);ctx.fillStyle='white';ctx.fillRect(0,0,128,128);half.draw(0,ctx,{unclipped:true});
              if(Math.abs(ctx.getImageData(52*scale,84*scale,1,1).data[0]-249)>1)throw Error('ancestor half opacity quantization');
              // The surface is painted after DROP, so translucent surface receives under-shadow contribution.
              ctx.fillStyle='rgba(200,120,80,0.25098039215686274)';ctx.fillRect(20,20,64,64);
              const p=ctx.getImageData(52*scale,52*scale,1,1).data;const rgb=[200,120,80];
              if(rgb.some((x,k)=>Math.abs(p[k]-(x*64/255+249*(1-64/255)))>2))throw Error('surface composition');
            }
            frames++;
          }
          return {frames};
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()]);assert result['frames']==16,result;print('PASS actual Canvas/WASM all named presets, DPR1/2 composition/opacity:',result);b.close()
if __name__=='__main__':main()
