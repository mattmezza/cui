#!/usr/bin/env python3
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Actual notification component commands through Canvas, light/dark DPR1/2."""
import argparse
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--chromium');p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);wasm=a.output/'notification.wasm'
    exports=['notification_shadow_scene','notification_shadow_type','notification_shadow_value','notification_shadow_paint','notification_shadow_palette','cui_canvas_shadow_values','cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width','cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound','cui_canvas_shadow_pixels']
    sources=[str(x) for x in (ROOT/'src').rglob('*.c') if 'text' not in x.relative_to(ROOT/'src').parts]+[str(ROOT/'web/modules/cui_shadow_bridge.c'),str(ROOT/'tests/web/notification_shadow_probe.c')]
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*sources,'-lm',*[f'-Wl,--export={x}' for x in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        b=p.chromium.launch(headless=True,**({'executable_path':a.chromium} if a.chromium else {}));page=b.new_page()
        result=page.evaluate('''async ([bytes,source])=>{
          const module=await WebAssembly.compile(new Uint8Array(bytes)),imports={};for(const i of WebAssembly.Module.imports(module))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();const m=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          let frames=0;
          for(const dark of [0,1])for(const scale of [1,2]){
            const count=e.notification_shadow_scene(dark,scale),commands=[];
            for(let i=0;i<count;i++){
              const type=e.notification_shadow_type(i),v=Array.from({length:14},(_,k)=>e.notification_shadow_value(i,k));
              if(type===7)commands.push({type:'shadow',values:Array.from({length:16},(_,k)=>e.notification_shadow_paint(i,k))});
              else commands.push({type:type===3?'clipPush':type===4?'clipPop':'other',drawType:type,values:v,bounds:v.slice(0,4)});
            }
            const shadows=commands.filter(c=>c.type==='shadow');if(shadows.length!==2)throw Error('component source pair missing');
            if(shadows.some((c,i)=>c.values[9]!==[10,4][i]||c.values[10]!==[7.5,3][i]||c.values[11]!==[-3,-4][i]||c.values[15]!==26))throw Error('ordered source layers');
            const canvas=new OffscreenCanvas(128,128),ctx=canvas.getContext('2d');ctx.setTransform(scale,0,0,scale,0,0);
            const raster=m.cuiShadowWasmRasterizer(e),renderer=m.createCuiShadowRenderer(raster),prepared=renderer.prepare(commands,ctx,128/scale,128/scale);
            ctx.fillStyle='white';ctx.fillRect(0,0,128/scale,128/scale);
            for(let i=0;i<commands.length;i++){
              const c=commands[i];if(c.type==='shadow'){prepared.draw(i,ctx,{unclipped:true});continue;}
              const v=c.values;if(c.drawType===0||c.drawType===1){ctx.beginPath();ctx.roundRect(v[0],v[1],v[2],v[3],Math.max(0,v[8]));ctx.fillStyle=`rgba(${v[4]},${v[5]},${v[6]},${v[7]/255})`;if(c.drawType===0)ctx.fill();else{ctx.strokeStyle=ctx.fillStyle;ctx.lineWidth=v[9];ctx.stroke();}}
              else if(c.drawType!==3&&c.drawType!==4)throw Error('unhandled actual component command '+c.drawType);
            }
            const center=ctx.getImageData(32*scale,28*scale,1,1).data,palette=Array.from({length:4},(_,k)=>e.notification_shadow_palette(k));if(center.some((x,k)=>x!==palette[k]))throw Error('actual surface palette');
            for(const [x,y] of [[32,42],[14,28],[50,28],[32,47]]){
              let expected=255;for(const c of shadows){const v=c.values.slice();v[15]=255;const q=raster(v,scale,0,0),ix=x*scale-Math.round(q.bounds[0]*scale),iy=y*scale-Math.round(q.bounds[1]*scale),alpha=ix>=0&&iy>=0&&ix<q.width&&iy<q.height?q.pixels[(iy*q.width+ix)*4+3]/255*26/255:0;expected*=1-alpha;}
              const pixel=ctx.getImageData(x*scale,y*scale,1,1).data;if(Array.from(pixel).slice(0,3).some(x=>Math.abs(x-expected)>2))throw Error('actual source falloff '+pixel+' expected '+expected);
            }
            const before=Array.from(ctx.getImageData(0,0,128,128).data);shadows[0].values[10]=NaN;let rejected=false;try{renderer.prepare(commands,ctx,128/scale,128/scale);}catch{rejected=true;}const after=ctx.getImageData(0,0,128,128).data;if(!rejected||before.some((x,i)=>x!==after[i]))throw Error('preclear rejection');frames++;
          }return {frames};
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()]);assert result['frames']==4,result;print('PASS actual Notification SOURCE Canvas light/dark DPR1/2 commands/pixels/preclear:',result);b.close()
if __name__=='__main__':main()
