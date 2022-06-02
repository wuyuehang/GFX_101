#version 460 core

layout (location = 0) in vec3 vPos;

layout (location = 0) out vec4 Pos_Viewspace;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);
    Pos_Viewspace = view_mat * model_mat * vec4(vPos, 1.0);
}
