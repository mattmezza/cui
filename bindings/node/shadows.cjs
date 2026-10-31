'use strict';
module.exports = native => {
 const number=(v,min,max)=>{if(typeof v!=='number'||!Number.isFinite(v)||!Number.isFinite(Math.fround(v))||v<min||v>max)throw new TypeError('invalid shadow numeric domain');return Math.fround(v);};
 const uint=v=>{if(!Number.isInteger(v)||v<0||v>0xffffffff)throw new TypeError('shadow color must be uint32 RGBA');return v;};
 const ShadowPart=Object.freeze({ROOT:0,MAIN:1,INDICATOR:2,TRACK:3,THUMB:4});
 const canonicalPart=(kind,part)=>{if(!Number.isInteger(part)||part<0||part>4)throw new TypeError("ShadowPart required");let roles,main;if([1,2,3,4,11,12].includes(kind)){roles=[0,1];main=0;}else if([5,6,20].includes(kind)){roles=[0,1,2];main=2;}else if(kind===7){roles=[0,1,3,4];main=3;}else if(kind===9){roles=[0,1,3];main=3;}else if(kind===17){roles=[0];}else roles=[];if(!roles.includes(part))throw new TypeError("unsupported shadow part");return part===1?main:part;};
 const ShadowKind=Object.freeze({DROP:0,INSET:1});
 const ShadowPreset=Object.freeze({NONE:0,XXS:1,XS:2,SM:3,MD:4,LG:5,XL:6,XXL:7});
 class CornerRadii {
  constructor(topLeft=0,topRight=0,bottomLeft=0,bottomRight=0){this.values=Object.freeze([topLeft,topRight,bottomLeft,bottomRight].map(v=>number(v,0,16384)));Object.freeze(this);}
 }
 class BoxShadow {
  constructor(offsetX=0,offsetY=0,sigma=0,spread=0,kind=0,rgba=0x00000040){
   if(kind!==0&&kind!==1)throw new TypeError('invalid shadow kind');
   this.values=Object.freeze([number(offsetX,-4096,4096),number(offsetY,-4096,4096),number(sigma,0,256),number(spread,-4096,4096),kind,uint(rgba)]);Object.freeze(this);
  }
  static css(offsetX=0,offsetY=0,blurRadius=0,spread=0,rgba=0x00000040,kind=0){
   const checked=new BoxShadow(offsetX,offsetY,0,spread,kind,rgba);
   return new BoxShadow(...native.shadowCSS(checked.values[0],checked.values[1],number(blurRadius,0,512),checked.values[3],rgba,kind));
  }
 }
 class ShadowStyle {
  constructor(layers=[],radii=new CornerRadii()){
   if(!Array.isArray(layers)||layers.length>8||!layers.every(v=>v instanceof BoxShadow)||!(radii instanceof CornerRadii))throw new TypeError('at most eight typed shadows and CornerRadii required');
   this.layers=Object.freeze([...layers]);this.radii=radii;Object.freeze(this);
  }
  wire(){return [...this.radii.values,...this.layers.flatMap(v=>v.values)];}
  static preset(preset,radii=new CornerRadii()){
   if(!Number.isInteger(preset)||preset<0||preset>7||!(radii instanceof CornerRadii))throw new TypeError('invalid shadow preset/radii');
   const v=native.shadowPreset(preset,radii.values),layers=[];
   for(let i=4;i<v.length;i+=6)layers.push(new BoxShadow(...v.slice(i,i+6)));
   return new ShadowStyle(layers,radii);
  }
 }
 return {BoxShadow,ShadowStyle,ShadowKind,ShadowPreset,CornerRadii,ShadowPart,canonicalPart};
};
