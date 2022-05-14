#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vOffset; // instance attribute offset
layout (location = 0) out vec2 fUV;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos + vOffset, 1.0);
    fUV = vUV;
}
