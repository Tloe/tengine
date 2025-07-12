#version 450

const vec2 VERTICES[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0) 
);

void main() {
    gl_Position = vec4(VERTICES[gl_VertexIndex], 0.0, 1.0);
}
