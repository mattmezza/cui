#version 450
layout(set=2,binding=0) uniform sampler2D glyphs;
layout(set=3,binding=0) uniform Params { vec4 bounds; vec4 viewport; vec4 color; vec4 shape; vec4 atlas; } p;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 result;
float rounded(vec2 point,vec2 halfSize,float radius) {
    vec2 q=abs(point)-halfSize+radius;
    return length(max(q,0))+min(max(q.x,q.y),0)-radius;
}
void main() {
    float alpha;
    if(p.atlas.z>0) alpha=texture(glyphs,p.atlas.xy+uv*p.atlas.zw).r;
    else {
        vec2 halfSize=p.bounds.zw*0.5;
        float radius=min(max(p.shape.z,0),min(halfSize.x,halfSize.y));
        vec2 point=(uv-0.5)*p.bounds.zw;
        float d=rounded(point,halfSize,radius);
        float aa=max(fwidth(d),0.5);
        alpha=1-smoothstep(-aa*0.5,aa*0.5,d);
        if(p.shape.w>0) {
            vec2 inner=max(halfSize-p.shape.w,vec2(0));
            float inside=rounded(point,inner,max(radius-p.shape.w,0));
            alpha*=smoothstep(-aa*0.5,aa*0.5,inside);
        }
    }
    result=vec4(p.color.rgb,p.color.a*alpha);
}
