/* SPDX-License-Identifier: MIT */
/* Pure C-backed, value-decoded shadow consumer. No target clearing here. */
export function cuiShadowWasmRasterizer(wasm) {
  function call(values, scale, ox, oy, raster, kind=0) {
    if(kind!==0 && kind!==1)throw Error('CUI invalid shadow kind');
    new Float32Array(wasm.memory.buffer, wasm.cui_canvas_shadow_values(), 16).set(values);
    const operation=kind===1?(raster?wasm.cui_canvas_inset_shadow_prepare:wasm.cui_canvas_inset_shadow_query):(raster?wasm.cui_canvas_shadow_prepare:wasm.cui_canvas_shadow_query);
    if (!operation || !operation(scale, ox, oy)) throw Error('CUI shadow raster rejected');
    const width = wasm.cui_canvas_shadow_width(), height = wasm.cui_canvas_shadow_height();
    const result = {width, height, bounds: Array.from({length: 4}, (_, i) => wasm.cui_canvas_shadow_bound(i)),
      coveredLayer: Array.from({length: 4}, (_, i) => wasm.cui_canvas_shadow_layer_bound(i))};
    if(kind===1){result.coveredHole=result.coveredLayer;result.coveredElement=Array.from({length:4},(_,i)=>wasm.cui_canvas_inset_shadow_element_bound(i));}
    if(raster)result.pixels = new Uint8ClampedArray(wasm.memory.buffer, wasm.cui_canvas_shadow_pixels(), width * height * 4);
    return result;
  }
  const fn = (v,s,x,y,k=0) => call(v,s,x,y,true,k);
  fn.query = (v,s,x,y,k=0) => call(v,s,x,y,false,k);
  return fn;
}
export function createCuiShadowRenderer(rasterize) {
  if(typeof rasterize?.query!=='function')throw Error('CUI shadow checked query required');
  const cache = new Map(), maxBytes = 32 * 1024 * 1024;
  let bytes = 0, frame = 0, hits = 0, misses = 0, activePrepared = null;
  function prepare(commands, ctx, width, height) {
    activePrepared?.clear(); activePrepared = null; const generation = ++frame;
    const t = ctx.getTransform();
    const shadows = commands.some(c => (c.type === 'shadow' || c.type === 'insetShadow'));
    if (!shadows) return {draw: () => false};
    const eps = 4 * 2 ** -23 * Math.max(Math.abs(t.a), Math.abs(t.d));
    if (![t.a,t.b,t.c,t.d,t.e,t.f].every(Number.isFinite) || t.a <= 0 || t.d <= 0 ||
        t.b !== 0 || t.c !== 0 || Math.abs(t.a - t.d) > eps) throw Error('CUI nonuniform shadow transform');
    if(![width,height].every(Number.isFinite) || width<=0 || height<=0 || commands.length>131072)throw Error('CUI invalid shadow frame');
    const scale = t.a, prepared = new Map(), shadowIndices = new Set(), stack = [];
    let clip = [0,0,width,height]; activePrepared = prepared;
    function intersect(a,b) {const x=Math.max(a[0],b[0]),y=Math.max(a[1],b[1]);return [x,y,Math.max(0,Math.min(a[0]+a[2],b[0]+b[2])-x),Math.max(0,Math.min(a[1]+a[3],b[1]+b[3])-y)];}
    function covered(b) {const x=Math.floor(b[0]*scale+t.e),y=Math.floor(b[1]*scale+t.f);return [x,y,Math.ceil((b[0]+b[2])*scale+t.e)-x,Math.ceil((b[1]+b[3])*scale+t.f)-y];}
    for (let i=0;i<commands.length;i++) {
      const c=commands[i];
      if(c.type==='clipPush'){if(!Array.isArray(c.bounds) || c.bounds.length!==4 || !c.bounds.every(Number.isFinite) || c.bounds[2]<0 || c.bounds[3]<0 || !Number.isFinite(c.bounds[0]+c.bounds[2]) || !Number.isFinite(c.bounds[1]+c.bounds[3]))throw Error('CUI invalid shadow clip');if(stack.length===64)throw Error('CUI shadow clip overflow');stack.push(clip);clip=intersect(clip,c.bounds);continue;}
      if(c.type==='clipPop'){if(!stack.length)throw Error('CUI shadow clip underflow');clip=stack.pop();continue;}
      if(c.type!=='shadow' && c.type!=='insetShadow')continue;
      const kind=c.type==='insetShadow'?1:0;
      shadowIndices.add(i);
      const v=Array.from(c.values);
      if(v.length!==16 || !v.every(Number.isFinite) || v.slice(12).some(x=>x<0 || x>255 || !Number.isInteger(x)))throw Error('CUI invalid shadow value');
      const dx=Math.floor(v[0]*scale+t.e),dy=Math.floor(v[1]*scale+t.f);
      const nx=Math.fround((v[0]*scale+t.e-dx)/scale),ny=Math.fround((v[1]*scale+t.f-dy)/scale);
      const alpha=v[15]/255;v[15]=255;
      const actual=rasterize.query(v,scale,t.e,t.f,kind),original=v.slice();
      const visible=intersect(actual.coveredLayer,covered(clip));
      v[0]=nx;v[1]=ny;
      let normalized=null;try{normalized=rasterize.query(v,scale,0,0,kind);}catch{/* Actual query was valid; normalization can exceed the public coordinate domain. */}
      let cacheOx=0,cacheOy=0;
      if(!normalized || actual.width!==normalized.width || actual.height!==normalized.height ||
         (kind===1 && (actual.coveredHole[2]!==normalized.coveredHole[2] || actual.coveredHole[3]!==normalized.coveredHole[3] ||
          actual.coveredHole[0]!==normalized.coveredHole[0]+dx || actual.coveredHole[1]!==normalized.coveredHole[1]+dy)) ||
         Math.round(actual.bounds[0]*scale+t.e)!==Math.round(normalized.bounds[0]*scale)+dx ||
         Math.round(actual.bounds[1]*scale+t.f)!==Math.round(normalized.bounds[1]*scale)+dy){
        v.splice(0,16,...original);cacheOx=t.e;cacheOy=t.f;
      }
      const key=JSON.stringify([kind,scale,cacheOx,cacheOy,...v]); let entry=cache.get(key);
      if(!entry){
        const image=rasterize(v,scale,cacheOx,cacheOy,kind), size=image.width*image.height*4;
        if(size>maxBytes || image.pixels.length!==size)throw Error('CUI shadow cache capacity');
        while(bytes+size>maxBytes || cache.size>=64){const old=[...cache].find(([,e])=>e.frame!==frame);if(!old)throw Error('CUI shadow frame cache capacity');cache.delete(old[0]);bytes-=old[1].bytes;}
        const canvas=new OffscreenCanvas(Math.max(1,image.width),Math.max(1,image.height));
        if(size)canvas.getContext('2d').putImageData(new ImageData(new Uint8ClampedArray(image.pixels),image.width,image.height),0,0);
        entry={canvas,bounds:image.bounds,width:image.width,height:image.height,bytes:size,frame};cache.set(key,entry);bytes+=size;misses++;
      }else{cache.delete(key);cache.set(key,entry);hits++;}
      entry.frame=frame;
      if(entry.width && visible[2]>0 && visible[3]>0 && alpha)prepared.set(i,{entry,alpha,actual,mask:covered(clip)});
    }
    if(stack.length)throw Error('CUI shadow unmatched clips');
    return {draw(index, target, options) {
      if(generation!==frame)throw Error('CUI shadow prepared frame expired');
      if(!shadowIndices.has(index))return false;
      if(options?.unclipped!==true)throw Error('CUI shadow draw requires unclipped target; prepared mask owns clipping');
      const p=prepared.get(index);if(!p)return true;
      const current=target.getTransform();
      if(['a','b','c','d','e','f'].some(k=>current[k]!==t[k]))throw Error('CUI shadow transform changed after preflight');
      target.save();target.globalAlpha*=p.alpha;target.imageSmoothingEnabled=false;
      const q=p.actual,qx=Math.round(q.bounds[0]*scale+t.e),qy=Math.round(q.bounds[1]*scale+t.f);
      const crop=intersect([qx,qy,q.width,q.height],p.mask);
      if(crop[2]>0 && crop[3]>0)target.drawImage(p.entry.canvas,crop[0]-qx,crop[1]-qy,crop[2],crop[3],
        (crop[0]-t.e)/scale,(crop[1]-t.f)/scale,crop[2]/scale,crop[3]/scale);
      target.restore();return true;
    }};
  }
  return {prepare,stats:()=>({bytes,hits,misses,entries:cache.size}),clear:()=>{activePrepared?.clear();activePrepared=null;cache.clear();bytes=0;frame++;}};
}
