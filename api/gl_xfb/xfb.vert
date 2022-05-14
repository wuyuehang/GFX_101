#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;

layout (location = 0) out vec3 xfbPos; // view space
layout (location = 1) out vec2 xfbUV;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);
    xfbPos = vec3(view_mat * model_mat * vec4(vPos, 1.0));
    xfbUV = vUV;
}
