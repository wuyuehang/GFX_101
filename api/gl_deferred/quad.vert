#version 460 core

const vec4 lookup[] = vec4[](
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0), // top-left-tri
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4(1.0, -1.0, 0.0, 1.0) // bottom-right-tri
);

layout (location = 0) out vec2 UV;

void main() {
    gl_Position = lookup[gl_VertexID];
    UV = 0.5 * gl_Position.xy + 0.5;
}