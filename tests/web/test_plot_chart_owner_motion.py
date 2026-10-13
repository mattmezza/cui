#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Standalone actual WASM owner motion, painted Chart reveal and Plot marks."""
import argparse
import ast
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import threading
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[2]
HTML='''<!doctype html><button id="reduced">Reduced motion</button><canvas width="460" height="220"></canvas>
<script>
const canvas=document.querySelector('canvas'),ctx=canvas.getContext('2d');let e,reduced=0,last=0;
const imports={wasi_snapshot_preview1:{proc_exit:c=>{throw Error(c)},fd_close:()=>8,fd_seek:()=>8,fd_fdstat_get:()=>8,
fd_write:(fd,p,n,w)=>{new DataView(e.memory.buffer).setUint32(w,0,true);return 0},
environ_sizes_get:(c,b)=>{const v=new DataView(e.memory.buffer);v.setUint32(c,0,true);v.setUint32(b,0,true);return 0},environ_get:()=>0,
clock_time_get:(c,p,t)=>{new DataView(e.memory.buffer).setBigUint64(t,BigInt(Math.round(performance.now()*1e6)),true);return 0},
random_get:(p,n)=>{crypto.getRandomValues(new Uint8Array(e.memory.buffer,p,n));return 0}}};
function paint(){ctx.clearRect(0,0,460,220);for(let i=0;i<e.probe_count();i++){
const f=n=>e.probe_command(i,n),[x,y,w,h]=[f(1),f(2),f(3),f(4)];ctx.fillStyle=`rgba(${f(5)},${f(6)},${f(7)},${f(8)/255})`;
if(f(0)===0)ctx.fillRect(x,y,w,h);
if(f(0)===2){ctx.font=`${f(9)}px sans-serif`;ctx.fillText(new TextDecoder().decode(new Uint8Array(e.memory.buffer,e.probe_text(i),e.probe_length(i))),x,y+f(9));}
if(f(0)===5){const bw=e.probe_bitmap(i,0),bh=e.probe_bitmap(i,1),stride=e.probe_bitmap(i,2),p=e.probe_pixels(i);if(!bw||!bh)continue;
const temp=document.createElement('canvas');temp.width=bw;temp.height=bh;const tc=temp.getContext('2d'),pixels=new Uint8ClampedArray(bw*bh*4);
for(let row=0;row<bh;row++)pixels.set(new Uint8Array(e.memory.buffer,p+row*stride,bw*4),row*bw*4);tc.putImageData(new ImageData(pixels,bw,bh),0,0);ctx.drawImage(temp,x,y,w,h);}}}
function frame(now){const dt=last?Math.min(.05,(now-last)/1000):0;last=now;if(!e.probe_frame(dt,reduced))throw Error('Probe frame failed');paint();requestAnimationFrame(frame);}
canvas.onmousemove=event=>{const r=canvas.getBoundingClientRect();e.probe_input(0,event.clientX-r.left,event.clientY-r.top)};
document.querySelector('#reduced').onclick=()=>{reduced=1};
WebAssembly.instantiateStreaming(fetch('probe.wasm'),imports).then(r=>{e=r.instance.exports;e._initialize();if(!e.probe_init())throw Error('init');window.probe=e;requestAnimationFrame(frame)});
</script>'''
def build(out):
    tree=ast.parse((ROOT/'scripts/build_web.py').read_text())
    sources=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='SOURCES'for t in n.targets))
    sources=[s for s in sources if s not in ['web/gallery.c','examples/composed_apps/app.c']]
    for source in ['src/components/cui_plot_motion.c','tests/web/plot_chart_owner_motion_probe.c']:
        if source not in sources: sources.append(source)
    assert subprocess.check_output(['zig','version'],text=True).strip()=='0.16.0'
    exports=['probe_init','probe_input','probe_frame','probe_count','probe_command','probe_text','probe_length','probe_pixels','probe_bitmap','probe_appearance']
    out.mkdir(parents=True,exist_ok=True)
    subprocess.run(['zig','cc','-target','wasm32-wasi','-mexec-model=reactor','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/clay'),*[str(ROOT/s)for s in sources],'-lm','-Wl,-z,stack-size=8388608',*[f'-Wl,--export={s}'for s in exports],'-o',str(out/'probe.wasm')],check=True)
    (out/'index.html').write_text(HTML)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'build/owner-motion-probe');p.add_argument('--chromium');a=p.parse_args();build(a.output)
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(a.output.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright()as pw:
            b=pw.chromium.launch(headless=True,**({'executable_path':a.chromium}if a.chromium else{}));page=b.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)));page.goto(f'http://127.0.0.1:{server.server_port}');page.wait_for_function('window.probe')
            page.wait_for_timeout(500)
            reveal=page.evaluate('[probe.probe_appearance(0),probe.probe_appearance(1)]');assert 0<reveal[1]<reveal[0]<1,reveal
            painted=page.evaluate('''()=>{const c=document.querySelector('canvas'),ctx=c.getContext('2d');return [0,1].map(i=>{const p=ctx.getImageData(10+220*i,100,200,100).data;let n=0;for(let j=0;j<p.length;j+=4)if(p[j+2]>p[j]+20&&p[j+3]>0)n++;return n;});}''');assert painted[0]>painted[1]>0,painted
            page.wait_for_timeout(550)
            reveal=page.evaluate('[probe.probe_appearance(0),probe.probe_appearance(1)]');assert reveal[0]==1 and reveal[1]<1,reveal
            # Each actual owner follows the same local pointer movement in turn.
            canvas=page.locator('canvas').bounding_box()
            def mark_x(owner):
                return page.evaluate('''(owner)=>{const e=probe;return Array.from({length:e.probe_count()},(_,i)=>[e.probe_command(i,0),e.probe_command(i,1),e.probe_command(i,3),e.probe_command(i,5)]).filter(c=>c[0]===0&&c[2]===6&&c[3]===255&&c[1]>=owner*220&&c[1]<owner*220+100).map(c=>c[1]-owner*220-10)[0];}''',owner)
            samples=[]
            for owner in [0,1]:
                page.mouse.move(canvas['x']+owner*220+25,canvas['y']+30);page.wait_for_timeout(150)
                page.mouse.move(canvas['x']+owner*220+60,canvas['y']+30);page.wait_for_timeout(150)
                samples.append(mark_x(owner))
            assert 10<samples[1]<samples[0]<=50,samples
            # Verify actual canvas contains the rendered red pointer mark.
            red=page.evaluate('''()=>{const c=document.querySelector('canvas'),p=c.getContext('2d').getImageData(0,0,460,80).data;let n=0;for(let i=0;i<p.length;i+=4)if(p[i]===255&&p[i+1]===0&&p[i+2]===0&&p[i+3]>0)n++;return n;}''');assert red>=50,red
            page.locator('#reduced').click();page.wait_for_timeout(80)
            assert page.evaluate('[probe.probe_appearance(0),probe.probe_appearance(1)]')==[1,1]
            assert not errors,errors;b.close()
    finally:server.shutdown()
    print('Plot Chart owner motion WASM browser passed')
if __name__=='__main__':main()
