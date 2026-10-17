# /// script
# dependencies = ["playwright==1.58.0"]
# ///
"""Real Canvas glyph-mask gradient pixels at three device densities."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
ROOT = Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--chromium',default=str(Path.home()/'.cache/ms-playwright/chromium-1208/chrome-linux64/chrome'))
    args=parser.parse_args()
    class Handler(SimpleHTTPRequestHandler):
        def log_message(self,*_args):pass
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(ROOT)))
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        with sync_playwright() as p:
            browser=p.chromium.launch(headless=True,executable_path=args.chromium)
            page=browser.new_page();page.goto(f'http://127.0.0.1:{server.server_port}/')
            results=page.evaluate('''async()=>{
             const {paintGradientText,sampleTextPaint}=await import('/web/modules/cui_text_paint.js');
             const stops=[[0,255,0,0,255],[1,0,0,255,255]],results=[];
             const alpha=sampleTextPaint({x0:0,y0:0,x1:1,y1:0,stops:[[0,255,0,0,0],[1,0,0,255,255]]},.5,0);
             if(String(alpha)!=='0,0,255,128')throw Error('premultiplied alpha');
             for(const scale of [1,1.5,2]){
              const canvas=document.createElement('canvas');canvas.width=320*scale;canvas.height=180*scale;
              const ctx=canvas.getContext('2d',{willReadFrequently:true});ctx.scale(scale,scale);
              ctx.font='96px monospace';ctx.textBaseline='alphabetic';ctx.fillStyle='orange';
              const savedFill=ctx.fillStyle;const paint={x0:10,y0:0,x1:75,y1:0,stops};
              paintGradientText(ctx,'M',10,110,paint);
              if(ctx.fillStyle!==savedFill||ctx.textBaseline!=='alphabetic')throw Error('context state');
              let red=0,blue=0;const data=ctx.getImageData(0,0,canvas.width,canvas.height).data;
              for(let i=0;i<data.length;i+=4){red+=data[i]>data[i+2]+20&&data[i+3]>20;blue+=data[i+2]>data[i]+20&&data[i+3]>20;}
              if(red<20||blue<20)throw Error('single glyph must contain both colors');
              ctx.clearRect(0,0,320,180);ctx.save();ctx.beginPath();ctx.rect(0,0,40,180);ctx.clip();
              paintGradientText(ctx,'M',10,110,paint);ctx.restore();
              const clipped=ctx.getImageData(40*scale,0,canvas.width-40*scale,canvas.height).data;
              if(clipped.some((v,i)=>i%4===3&&v))throw Error('clip');
              // A line mask must retain the same coverage as a full shaped italic
              // fillText, including ink overhang, combining marks and ligatures.
              ctx.clearRect(0,0,320,180);ctx.font='italic 64px serif';
              paintGradientText(ctx,'fj ä fi',30,100,{...paint,stops:[[0,255,255,255,255],[1,255,255,255,255]]});
              const actual=ctx.getImageData(0,0,canvas.width,canvas.height).data;
              ctx.clearRect(0,0,320,180);ctx.fillStyle='white';ctx.fillText('fj ä fi',30,100);
              const expected=ctx.getImageData(0,0,canvas.width,canvas.height).data;
              let differences=0;for(let i=3;i<actual.length;i+=4)differences+=Math.abs(actual[i]-expected[i])>2;
              if(differences>20)throw Error('mask lost contextual coverage '+differences);
              results.push({scale,red,blue,differences});
             }
             // Two separately shaped rows share a block-anchored vertical field.
             const multi=document.createElement('canvas');multi.width=240;multi.height=160;
             const mc=multi.getContext('2d',{willReadFrequently:true});mc.font='48px monospace';
             mc.textBaseline='alphabetic';
             const vertical={x0:0,y0:0,x1:0,y1:160,stops};
             paintGradientText(mc,'MMM',10,60,vertical);
             paintGradientText(mc,'MMM',10,140,vertical);
             const md=mc.getImageData(0,0,240,160).data;let upper=0,lower=0;
             for(let y=0;y<160;y++)for(let x=0;x<240;x++){
              const at=(y*240+x)*4;if(md[at+3]<100)continue;
              if(y<70)upper+=md[at]>md[at+2];else lower+=md[at+2]>md[at];
             }
             if(upper<100||lower<100)throw Error('multiline block anchoring');
             const hard={x0:0,y0:0,x1:1,y1:1,stops:[[0,255,0,0,255],[.5,255,0,0,255],[.5,0,0,255,255],[1,0,0,255,255]]};
             if(sampleTextPaint(hard,.5,.5)[2]!==255)throw Error('diagonal hard edge');
             return results;
            }''')
            browser.close();print('Canvas text gradient PASS',results)
    finally:server.shutdown();server.server_close()
if __name__=='__main__':main()
