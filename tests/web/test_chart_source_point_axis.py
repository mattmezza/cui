# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Pinned WASM source point-axis raster geometry versus legacy canvas pixels."""
import argparse
from pathlib import Path
import subprocess
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--chromium');parser.add_argument('--output',type=Path,default=ROOT/'build/chart-source-point-probe')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    wasm=out/'probe.wasm'
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-DCUI_CHART_BASE_ONLY=1','-I',str(ROOT/'include'),str(ROOT/'tests/web/chart_source_point_probe.c'),str(ROOT/'src/base/cui_chart.c'),str(ROOT/'src/base/cui_plot.c'),'-lm','-Wl,--export=probe_render','-Wl,--export=probe_pixels','-o',str(wasm)],check=True)
    with sync_playwright() as p:
        browser=p.chromium.launch(headless=True,**({'executable_path':args.chromium} if args.chromium else {}))
        page=browser.new_page();page.set_content('<canvas width="128" height="96"></canvas>')
        results=page.evaluate('''async bytes=>{
          const m=await WebAssembly.compile(new Uint8Array(bytes)),imports={};
          for(const i of WebAssembly.Module.imports(m))(imports[i.module]??={})[i.name]=()=>0;
          const e=new WebAssembly.Instance(m,imports).exports;e._initialize?.();
          const ctx=document.querySelector('canvas').getContext('2d');
          const sample=(mode,count)=>{if(e.probe_render(mode,count))throw Error('render');
            ctx.putImageData(new ImageData(new Uint8ClampedArray(e.memory.buffer,e.probe_pixels(),128*96*4),128,96),0,0);
            const at=(x,y)=>Array.from(ctx.getImageData(x,y,1,1).data);
            return [at(127,76),at(127,78),at(64,78)];};
          return [sample(0,3),sample(1,1),sample(1,5),e.probe_render(0,1)];
        }''',list(wasm.read_bytes()))
        red=lambda c:c[0]>200 and c[1]<50
        assert red(results[0][0]) and not red(results[0][1]),results
        assert red(results[1][1]) and not red(results[1][2]),results
        assert red(results[2][2]) and results[3]!=0,results
        page.screenshot(path=str(out/'source-point.png'));browser.close()
    print('PASS source point-axis WASM canvas raster and legacy rejection')
if __name__=='__main__':main()
