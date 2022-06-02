#version 460 core

layout (location = 0) in vec4 Pos_Viewspace;

layout (location = 0) out vec4 SV_Target;

uniform mat4 proj_mat;

void main() {
    SV_Target = Pos_Viewspace;
}