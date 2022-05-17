#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;
layout (location = 0) out vec2 fUV;

uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * vec4(vPos,1.0);
    fUV = vUV;
}
