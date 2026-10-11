#version 450
layout(set=1,binding=0) uniform Params { vec4 bounds; vec4 viewport; vec4 color; vec4 shape; vec4 atlas; } p;
layout(location=0) out vec2 uv;
void main() {
    vec2 corners[6]=vec2[](vec2(0,0),vec2(1,0),vec2(0,1),vec2(0,1),vec2(1,0),vec2(1,1));
    uv=corners[gl_VertexIndex];
    vec2 position=p.bounds.xy+uv*p.bounds.zw;
    gl_Position=vec4(position/p.viewport.xy*vec2(2,-2)+vec2(-1,1),0,1);
}
