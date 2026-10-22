/* SPDX-License-Identifier: MIT */
/* Physical-pixel analytic triangle consumer; no target mutation in prepare. */
export function createCuiMeshRenderer() {
  let generation = 0,activePrepared=null;
  const limit = {commands:131072, vertices:262144, canvases:4096, bytes:32*1024*1024, visits:100000000};
  const fail = reason => { activePrepared?.clear(); throw Error(`CUI mesh ${reason}`); };
  const finite = a => a.every(Number.isFinite);
  const edge = (a,b,x,y) => (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);
  const topLeft = (a,b) => b[1]<a[1] || (b[1]===a[1] && b[0]>a[0]);
  const intersect = (a,b) => {const x=Math.max(a[0],b[0]),y=Math.max(a[1],b[1]);return [x,y,Math.max(0,Math.min(a[0]+a[2],b[0]+b[2])-x),Math.max(0,Math.min(a[1]+a[3],b[1]+b[3])-y)];};
  const channel = (p,a) => {const l=Math.max(0,Math.min(1,a>0?p/a:0));return Math.round(255*(l<=.0031308?12.92*l:1.055*l**(1/2.4)-.055));};
  function prepare(commands,ctx,width,height) {
    activePrepared?.clear();
    const frame=++generation, prepared=new Map();
    activePrepared=prepared;
    if(!Array.isArray(commands) || commands.length>limit.commands)fail('command capacity');
    if(!commands.some(c=>c.type==='mesh'))return {draw:()=>false};
    const t=ctx.getTransform(),transform=[t.a,t.b,t.c,t.d,t.e,t.f];
    if(!finite(transform) || t.a<=0 || t.d!==t.a || t.b!==0 || t.c!==0)fail('unsupported transform');
    const alpha=ctx.globalAlpha,composite=ctx.globalCompositeOperation;
    if(!Number.isFinite(alpha) || alpha<0 || alpha>1 || composite!=='source-over')fail('unsupported composition');
    if(!finite([width,height]) || width<=0 || height<=0)fail('invalid viewport');
    const physical=[Math.round(width*t.a),Math.round(height*t.a)];
    if(!finite(physical) || physical.some(x=>x<1 || x>16384))fail('viewport capacity');
    let clip=[0,0,...physical],vertices=0,objects=0,bytes=0,visits=0,scratchBytes=0,scratch=null;
    const stack=[];
    for(let index=0;index<commands.length;index++) {
      const c=commands[index];
      if(c.type==='clipPush') {
        const b=c.bounds;
        if(!Array.isArray(b)||b.length!==4||!finite(b)||b[2]<0||b[3]<0||!finite([b[0]+b[2],b[1]+b[3]]))fail('invalid clip');
        if(stack.length===64)fail('clip capacity');
        const p=[b[0]*t.a+t.e,b[1]*t.a+t.f,b[2]*t.a,b[3]*t.a];
        if(!finite(p)||!finite([p[0]+p[2],p[1]+p[3]]))fail('clip overflow');
        stack.push(clip);clip=intersect(clip,p);continue;
      }
      if(c.type==='clipPop'){if(!stack.length)fail('clip underflow');clip=stack.pop();continue;}
      if(c.type!=='mesh')continue;
      const p=c.mesh,n=p?.vertexCount;
      if(!Number.isSafeInteger(n)||n<=0||n%3||n>limit.vertices-vertices)fail('vertex capacity');
      vertices+=n;
      if(!Array.isArray(p.vertices)||p.vertices.length!==n*4||!finite(p.vertices)||
         !Array.isArray(p.color)||p.color.length!==4||!finite(p.color)||p.color[3]<0||p.color[3]>1||
         !finite([p.originX,p.originY,p.scale])||p.scale<=0)fail('invalid paint');
      const color=p.color.slice(),rgb=color.slice(0,3).map(x=>channel(x,color[3])),rasters=[];
      prepared.set(index,rasters);
      for(let i=0;i<n;i+=3) {
        const v=Array.from({length:3},(_,j)=>{const k=(i+j)*4;return [(p.originX+p.scale*p.vertices[k])*t.a+t.e,(p.originY+p.scale*p.vertices[k+1])*t.a+t.f,p.vertices[k+2],p.vertices[k+3]];});
        if(!v.every(finite))fail('coordinate overflow');
        let area=edge(v[0],v[1],v[2][0],v[2][1]);
        if(!Number.isFinite(area))fail('area overflow');
        if(area===0)continue;
        if(area<0){[v[1],v[2]]=[v[2],v[1]];area=-area;}
        const lowX=Math.max(clip[0],Math.min(...v.map(q=>q[0]))),lowY=Math.max(clip[1],Math.min(...v.map(q=>q[1]))),
          highX=Math.min(clip[0]+clip[2],Math.max(...v.map(q=>q[0]))),highY=Math.min(clip[1]+clip[3],Math.max(...v.map(q=>q[1])));
        if(lowX>=highX || lowY>=highY)continue;
        const x0=Math.floor(lowX),y0=Math.floor(lowY),w=Math.ceil(highX)-x0,h=Math.ceil(highY)-y0,size=w*h*4;
        visits+=w*h;
        // Growing scratch briefly owns both old and new arrays. Include that
        // allocation peak as well as the newly uploaded canvas backing.
        const allocationScratch=size>scratchBytes?size+scratchBytes:scratchBytes;
        if(++objects>limit.canvases || visits>limit.visits || bytes+size+allocationScratch>limit.bytes)fail('raster capacity');
        bytes+=size;
        if(size>scratchBytes){scratch=new Uint8ClampedArray(size);scratchBytes=size;}
        else scratch.fill(0,0,size);
        const sx=((v[1][2]-v[0][2])*(v[2][1]-v[0][1])-(v[2][2]-v[0][2])*(v[1][1]-v[0][1]))/area,
          sy=((v[1][0]-v[0][0])*(v[2][2]-v[0][2])-(v[2][0]-v[0][0])*(v[1][2]-v[0][2]))/area,
          tx=((v[1][3]-v[0][3])*(v[2][1]-v[0][1])-(v[2][3]-v[0][3])*(v[1][1]-v[0][1]))/area,
          ty=((v[1][0]-v[0][0])*(v[2][3]-v[0][3])-(v[2][0]-v[0][0])*(v[1][3]-v[0][3]))/area;
        if(!finite([sx,sy,tx,ty]))fail('derivative overflow');
        const solid=Math.hypot(sx,sy)<.001;
        for(let y=y0;y<y0+h;y++)for(let x=x0;x<x0+w;x++) {
          if(x+.5<clip[0]||y+.5<clip[1]||x+.5>=clip[0]+clip[2]||y+.5>=clip[1]+clip[3])continue;
          const e=[edge(v[1],v[2],x+.5,y+.5),edge(v[2],v[0],x+.5,y+.5),edge(v[0],v[1],x+.5,y+.5)];
          if(!finite(e))fail('edge overflow');
          if(e.some((q,j)=>q<0||(q===0&&!topLeft(v[(j+1)%3],v[(j+2)%3]))))continue;
          let coverage=1;
          if(!solid){const s=e.reduce((a,q,j)=>a+q/area*v[j][2],0),tt=e.reduce((a,q,j)=>a+q/area*v[j][3],0),f=s*s-tt,g=Math.hypot(2*s*sx-tx,2*s*sy-ty);
            if(!finite([s,tt,f,g]))fail('coverage overflow');
            coverage=g>0?Math.max(0,Math.min(1,.5-f/g)):f<0?1:f>0?0:.5;}
          const a=Math.round(color[3]*coverage*255),k=((y-y0)*w+x-x0)*4;
          if(a){scratch[k]=rgb[0];scratch[k+1]=rgb[1];scratch[k+2]=rgb[2];scratch[k+3]=a;}
        }
        const canvas=new OffscreenCanvas(w,h),upload=canvas.getContext('2d');
        if(!upload)fail('canvas allocation');
        upload.putImageData(new ImageData(scratch.subarray(0,size),w,h),0,0);
        rasters.push({canvas,x:x0,y:y0});
      }
    }
    if(stack.length)fail('unmatched clips');
    scratch=null;
    return {draw(index,target,options) {
      if(frame!==generation)fail('prepared frame expired');
      if(!prepared.has(index))return false;
      if(options?.unclipped!==true)fail('draw requires unclipped target');
      const current=target.getTransform();
      if(['a','b','c','d','e','f'].some((k,i)=>current[k]!==transform[i])||target.globalAlpha!==alpha||target.globalCompositeOperation!==composite)fail('state changed after preflight');
      target.save();target.setTransform(1,0,0,1,0,0);target.imageSmoothingEnabled=false;
      for(const r of prepared.get(index))target.drawImage(r.canvas,r.x,r.y);
      target.restore();return true;
    },stats:()=>({vertices,objects,bytes:bytes+scratchBytes,visits})};
  }
  return {prepare};
}
