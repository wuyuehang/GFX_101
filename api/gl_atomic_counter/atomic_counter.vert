#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;

layout (location = 0) out vec2 UV;

layout (binding = 0, offset = 0) uniform atomic_uint acVertice;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = view_mat * model_mat * vec4(vPos, 1.0);
    UV = vUV;
    atomicCounterIncrement(acVertice);
}
