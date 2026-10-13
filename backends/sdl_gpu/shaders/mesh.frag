#version 450
layout(set=3,binding=0) uniform Params { vec4 bounds; vec4 viewport; vec4 color; vec4 shape; vec4 atlas; vec4 radii; vec4 edges; } p;
layout(location=0) in vec2 st;
layout(location=0) out vec4 result;
vec3 encodeSRGB(vec3 v) {
    return mix(12.92*v,1.055*pow(v,vec3(1.0/2.4))-0.055,greaterThan(v,vec3(0.0031308)));
}
void main() {
    float coverage=1.0;
    vec2 ds=vec2(dFdx(st.x),dFdy(st.x));
    vec2 dt=vec2(dFdx(st.y),dFdy(st.y));
    if(length(ds)>=0.001) {
        float f=st.x*st.x-st.y;
        float gradient=length(2.0*st.x*ds-dt);
        coverage=clamp(0.5-f/gradient,0.0,1.0);
    }
    float a=p.color.a*coverage;
    vec3 rgb=p.color.a>0.0?encodeSRGB(clamp(p.color.rgb/p.color.a,0.0,1.0)):vec3(0);
    result=vec4(rgb*a,a);
}
