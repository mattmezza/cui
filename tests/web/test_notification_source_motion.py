#!/usr/bin/env python3
# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Controlled source Notification enter/exit motion through actual WASM/Canvas pixels."""
import argparse
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--chromium');p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);wasm=a.output/'notification.wasm'
    exports=['notification_source_motion_scene','notification_source_motion_bound','notification_shadow_type','notification_shadow_value','notification_shadow_paint','notification_shadow_palette','cui_canvas_shadow_values','cui_canvas_shadow_query','cui_canvas_shadow_prepare','cui_canvas_shadow_width','cui_canvas_shadow_height','cui_canvas_shadow_layer_bound','cui_canvas_shadow_bound','cui_canvas_shadow_pixels']
    sources=[str(x) for x in (ROOT/'src').rglob('*.c') if x.relative_to(ROOT/'src').parts[0] in ('core', 'base', 'components', 'theme', 'effects')]+[str(ROOT/'web/modules/cui_shadow_bridge.c'),str(ROOT/'tests/web/notification_source_motion_probe.c')]
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*sources,'-lm',*[f'-Wl,--export={x}' for x in exports],'-o',str(wasm)],check=True)
    with sync_playwright() as p:
        b=p.chromium.launch(headless=True,**({'executable_path':a.chromium} if a.chromium else {}));page=b.new_page()
        result=page.evaluate('''async ([bytes,source])=>{
          const module=await WebAssembly.compile(new Uint8Array(bytes)),imports={};for(const i of WebAssembly.Module.imports(module))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(module,imports).exports;e._initialize?.();const m=await import(URL.createObjectURL(new Blob([source],{type:'text/javascript'})));
          const ease=(p)=>{let lo=0,hi=1;for(let i=0;i<60;i++){const u=(lo+hi)/2,x=3*(1-u)**2*u*.25+3*(1-u)*u*u*.25+u**3;if(x<p)lo=u;else hi=u;}const u=(lo+hi)/2;return 3*(1-u)**2*u*.1+3*(1-u)*u*u+u**3;};
          let frames=0;
          for(const scale of [1,2])for(const anchor of [0,1,2,3,4,5,6,7])for(const closing of [0,1])for(const reduced of [0,1]){
            const elapsed=closing?.05:.1,eased=reduced?1:ease(.25),direction=anchor>=3&&anchor<=5?1:anchor>=6?0:-1;
            const count=e.notification_source_motion_scene(anchor,closing,elapsed,scale,0,reduced),commands=[];
            const bound=Array.from({length:5},(_,k)=>e.notification_source_motion_bound(k));
            const expectedY=bound[4]+direction*(closing?eased:1-eased)*96;
            if(Math.abs(bound[1]-expectedY)>.002)throw Error('source96 geometry '+bound+' expected '+expectedY);
            for(let i=0;i<count;i++){
              const type=e.notification_shadow_type(i),v=Array.from({length:14},(_,k)=>e.notification_shadow_value(i,k));
              if(type===7)commands.push({type:'shadow',values:Array.from({length:16},(_,k)=>e.notification_shadow_paint(i,k))});
              else commands.push({type:type===3?'clipPush':type===4?'clipPop':'other',drawType:type,values:v,bounds:v.slice(0,4)});
            }
            const shadows=commands.filter(c=>c.type==='shadow');
            if(shadows.some(c=>Math.abs(c.values[1]-expectedY)>.002))throw Error('shadow did not follow source caster');
            const canvas=new OffscreenCanvas(384,384),ctx=canvas.getContext('2d');ctx.setTransform(scale,0,0,scale,0,0);
            const raster=m.cuiShadowWasmRasterizer(e),renderer=m.createCuiShadowRenderer(raster),prepared=renderer.prepare(commands,ctx,384/scale,384/scale);
            ctx.fillStyle='white';ctx.fillRect(0,0,384/scale,384/scale);
            for(let i=0;i<commands.length;i++){
              const c=commands[i];if(c.type==='shadow'){prepared.draw(i,ctx,{unclipped:true});continue;}
              const v=c.values;if(c.drawType===0||c.drawType===1){ctx.beginPath();ctx.roundRect(v[0],v[1],v[2],v[3],Math.max(0,v[8]));ctx.fillStyle=`rgba(${v[4]},${v[5]},${v[6]},${v[7]/255})`;if(c.drawType===0)ctx.fill();else{ctx.strokeStyle=ctx.fillStyle;ctx.lineWidth=v[9];ctx.stroke();}}
              else if(c.drawType!==3&&c.drawType!==4)throw Error('unhandled actual component command '+c.drawType);
            }
            const alpha=reduced?(closing?0:1):(closing?1-eased:eased);
            const x=Math.floor((bound[0]+bound[2]/2)*scale),y=Math.floor((bound[1]+bound[3]/2)*scale);
            if(x>=0&&y>=0&&x<384&&y<384){
              const pixel=ctx.getImageData(x,y,1,1).data,palette=Array.from({length:3},(_,k)=>e.notification_shadow_palette(k));
              let under=255;for(const c of shadows){const q=raster(c.values,scale,0,0),ix=x-Math.round(q.bounds[0]*scale),iy=y-Math.round(q.bounds[1]*scale);const a=ix>=0&&iy>=0&&ix<q.width&&iy<q.height?q.pixels[(iy*q.width+ix)*4+3]/255:0;under*=1-a;}
              const ink=Math.floor(alpha*255)/255;
              if(palette.some((v,k)=>Math.abs(pixel[k]-(v*ink+under*(1-ink)))>2))throw Error('source motion surface/shadow pixel '+pixel+' alpha '+alpha);
            }
            if(!reduced&&direction){
              e.notification_source_motion_scene(anchor,closing,elapsed,scale,1,0);
              const legacyY=e.notification_source_motion_bound(1),expectedLegacy=bound[4]+direction*(closing?eased:1-eased)*16;
              if(Math.abs(legacyY-expectedLegacy)>.002||Math.abs(legacyY-bound[1])<20)throw Error('legacy/source translation distinction');
            }
            frames++;
          }return {frames};
        }''',[list(wasm.read_bytes()),(ROOT/'web/modules/cui_shadow.js').read_text()]);assert result['frames']==64,result;print('PASS controlled actual Notification SOURCE Canvas64 enter/exit/placement/reduced/legacy DPR1/2 frames:',result);b.close()
if __name__=='__main__':main()
