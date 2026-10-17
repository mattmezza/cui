/* SPDX-License-Identifier: MIT */
// Continuous glyph-mask paint. Stops/axis are already resolved by Core.
export function sampleTextPaint(p,x,y){
 const dx=p.x1-p.x0,dy=p.y1-p.y0,length=dx*dx+dy*dy;
 const t=length?((x-p.x0)*dx+(y-p.y0)*dy)/length:1;
 let right=0;while(right<p.stops.length&&p.stops[right][0]<=t)right++;
 if(!right)return p.stops[0].slice(1);
 if(right===p.stops.length)return p.stops[right-1].slice(1);
 const a=p.stops[right-1],b=p.stops[right],f=(t-a[0])/(b[0]-a[0]),alpha=a[4]+(b[4]-a[4])*f;
 if(alpha<=0)return [0,0,0,0];
 return [1,2,3].map(i=>Math.round(((1-f)*a[i]*a[4]+f*b[i]*b[4])/alpha)).concat(Math.round(alpha));
}
export function validTextPaint(p){
 if(!p||![p.x0,p.y0,p.x1,p.y1].every(Number.isFinite)||!Array.isArray(p.stops)||p.stops.length<2||p.stops.length>8)return false;
 let previous=0;for(const s of p.stops){if(s.length!==5||!s.every(Number.isFinite)||s[0]<previous||s[0]>1||s.slice(1).some(v=>v<0||v>255||!Number.isInteger(v)))return false;previous=s[0];}
 return true;
}
export function paintGradientText(ctx,text,x,y,paint){
 if(!validTextPaint(paint))throw Error('Invalid CUI linear text paint');
 const transform=ctx.getTransform(),sx=transform.a,sy=transform.d;
 if(transform.b||transform.c||!(sx>0&&sy>0))throw Error('Unsupported CUI text paint transform');
 if(!text)return;
 const metrics=ctx.measureText(text);
 const left=(Math.floor((x-metrics.actualBoundingBoxLeft)*sx+transform.e)-transform.e)/sx;
 const top=(Math.floor((y-metrics.actualBoundingBoxAscent)*sy+transform.f)-transform.f)/sy;
 const width=Math.ceil((x+metrics.actualBoundingBoxRight-left)*sx);
 const height=Math.ceil((y+metrics.actualBoundingBoxDescent-top)*sy);
 if(width<=0||height<=0)return;
 if(!Number.isFinite(width)||!Number.isFinite(height)||width*height>1048576)throw Error('CUI text paint mask exceeds 4 MiB');
 const mask=new OffscreenCanvas(width,height),out=mask.getContext('2d',{willReadFrequently:true});
 out.font=ctx.font;out.textBaseline=ctx.textBaseline;out.textAlign=ctx.textAlign;out.direction=ctx.direction;
 out.fontKerning=ctx.fontKerning;out.fontStretch=ctx.fontStretch;out.fontVariantCaps=ctx.fontVariantCaps;
 out.setTransform(sx,0,0,sy,-left*sx,-top*sy);out.fillStyle='white';out.fillText(text,x,y);
 const pixels=out.getImageData(0,0,width,height),data=pixels.data;
 for(let j=0;j<height;j++)for(let i=0;i<width;i++){
  const offset=(j*width+i)*4,coverage=data[offset+3];
  if(!coverage)continue;
  const color=sampleTextPaint(paint,left+(i+.5)/sx,top+(j+.5)/sy);
  data[offset]=color[0];data[offset+1]=color[1];data[offset+2]=color[2];data[offset+3]=Math.floor(color[3]*coverage/255);
 }
 out.putImageData(pixels,0,0);
 ctx.drawImage(mask,left,top,width/sx,height/sy);
}
