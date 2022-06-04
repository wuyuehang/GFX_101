#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTan;
layout (location = 4) in vec3 vBta;

layout (location = 0) out vec3 Pos_viewspace;
layout (location = 1) out vec2 UV;

layout (location = 2) out vec3 Tan_viewspace;
layout (location = 3) out vec3 Bta_viewspace;
layout (location = 4) out vec3 Nor_viewspace;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);

    // calculate in viewspace
    Pos_viewspace = vec3(view_mat * model_mat * vec4(vPos, 1.0));

    // passthrough
    UV = vUV;

    Tan_viewspace = mat3(view_mat * model_mat) * vTan;
    Bta_viewspace = mat3(view_mat * model_mat) * vBta;
    Nor_viewspace = mat3(view_mat * model_mat) * vNor;
}