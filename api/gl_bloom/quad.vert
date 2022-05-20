#version 460 core

layout (location = 0) out vec2 UV;

const vec4 lookup[] = vec4[](
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0), // top-left-tri
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4(1.0, -1.0, 0.0, 1.0) // bottom-right-tri
);

void main() {
    gl_Position = lookup[gl_VertexID];
    UV = vec2(0.5 * gl_Position + 0.5);
}