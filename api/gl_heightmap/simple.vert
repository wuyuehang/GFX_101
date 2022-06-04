#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTan;
layout (location = 4) in vec3 vBta;

layout (location = 0) out vec3 Pos_viewspace;
layout (location = 1) out vec3 Nor_viewspace;
layout (location = 2) out vec2 UV;
layout (location = 3) out vec3 LightDir_viewspace;

uniform vec3 LightPos_viewspace;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);

    // calculate object position in viewspace
    Pos_viewspace = vec3(view_mat * model_mat * vec4(vPos, 1.0));

    // calculate normal in viewspace
    Nor_viewspace = mat3(transpose(inverse(view_mat * model_mat))) * vNor;

    // calculate light direction in viewspace
    LightDir_viewspace = LightPos_viewspace - Pos_viewspace;

    // passthrough
    UV = vUV;
}
