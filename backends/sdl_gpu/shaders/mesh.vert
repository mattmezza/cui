#version 450
layout(set=1,binding=0) uniform Params { vec4 bounds; vec4 viewport; vec4 color; vec4 shape; vec4 atlas; vec4 radii; vec4 edges; } p;
layout(location=0) in vec2 position;
layout(location=1) in vec2 source_st;
layout(location=0) out vec2 st;
void main() {
    st=source_st;
    gl_Position=vec4(position/p.viewport.xy*vec2(2,-2)+vec2(-1,1),0,1);
}
