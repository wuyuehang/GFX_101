#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;

layout (location = 0) out vec3 vout_Pos;

uniform mat4 model_mat;

void main() {
    // postpone projection/view transform to later stage
    vout_Pos = vec3(model_mat * vec4(vPos, 1.0));
}
