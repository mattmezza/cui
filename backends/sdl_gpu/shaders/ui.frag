#version 450
layout(set=2,binding=0) uniform sampler2D glyphs;
layout(set=3,binding=0) uniform Params { vec4 bounds; vec4 viewport; vec4 color; vec4 shape; vec4 atlas; vec4 radii; vec4 edges; } p;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 result;
float arcDistance(vec2 p,vec2 r) {
    if(r.x<=0.0||r.y<=0.0||p.x>=r.x||p.y>=r.y) return -min(p.x,p.y);
    return (length((p-r)/r)-1.0)*min(r.x,r.y);
}
float outline(vec2 p,vec2 size,vec4 rx,vec4 ry) {
    float d=-1e6;
    d=max(d,arcDistance(p,vec2(rx.x,ry.x)));
    d=max(d,arcDistance(vec2(size.x-p.x,p.y),vec2(rx.y,ry.y)));
    d=max(d,arcDistance(vec2(p.x,size.y-p.y),vec2(rx.z,ry.z)));
    return max(d,arcDistance(size-p,vec2(rx.w,ry.w)));
}
float shapeCoverage(vec2 p,vec2 size,vec4 radii,vec4 edges,float border) {
    float d=outline(p,size,radii,radii);
    float aa=max(fwidth(d),0.5);
    float alpha=1.0-smoothstep(-aa*0.5,aa*0.5,d); bool curved=(p.x<radii.x&&p.y<radii.x)||(size.x-p.x<radii.y&&p.y<radii.y)||(p.x<radii.z&&size.y-p.y<radii.z)||(size.x-p.x<radii.w&&size.y-p.y<radii.w); if(!curved) alpha=1.0;
    if(border>0.5) { if(dot(edges,vec4(1))<=0.0) return 0.0;
        vec2 inner=size-vec2(edges.x+edges.y,edges.z+edges.w);
        if(inner.x>0.0&&inner.y>0.0) {
            vec4 rx=max(radii-vec4(edges.x,edges.y,edges.x,edges.y),vec4(0));
            vec4 ry=max(radii-vec4(edges.z,edges.z,edges.w,edges.w),vec4(0));
            vec2 ip=p-vec2(edges.x,edges.z);
            float di=max(outline(ip,inner,rx,ry),max(max(-ip.x,-ip.y),max(ip.x-inner.x,ip.y-inner.y)));
            float ai=max(fwidth(di),0.5);
            alpha=max(0.0,alpha-(1.0-smoothstep(-ai*0.5,ai*0.5,di)));
        }
    }
    return alpha;
}
void main() {
    if(p.shape.z<0) { result=texture(glyphs,uv)*p.color; return; }
    float alpha;
    if(p.atlas.z>0) alpha=texture(glyphs,p.atlas.xy+uv*p.atlas.zw).r;
    else alpha=shapeCoverage(uv*p.bounds.zw,p.bounds.zw,p.radii,p.edges,p.shape.w);
    result=vec4(p.color.rgb,p.color.a*alpha);
}
