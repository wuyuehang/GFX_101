#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec3 vUV;

uniform MVP_ {
    mat4 model_mat;
    mat4 view_mat;
    mat4 proj_mat;
} MVP;

void main() {
    gl_Position = MVP.proj_mat * MVP.view_mat * MVP.model_mat * vec4(vPos, 1.0);
}