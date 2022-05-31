#version 460 core

layout (location = 0) in vec4 Pos_Viewspace;
layout (location = 1) in vec4 Pos_Projspace;

layout (location = 0) out vec4 SV_Target0;
layout (location = 1) out vec4 SV_Target1;
layout (location = 2) out vec4 SV_Target2;
layout (location = 3) out vec4 SV_Target3;

uniform mat4 proj_mat;

void main() {
    SV_Target0 = Pos_Viewspace;
    SV_Target1 = Pos_Projspace;

    // interpolate along viewspace
    vec4 pos_proj_div = (proj_mat * Pos_Viewspace);
    pos_proj_div.xyz = pos_proj_div.xyz / pos_proj_div.w;
    SV_Target2 = pos_proj_div;

    // remap [-1, 1] to [0, 1]
    SV_Target3.xyz = 0.5 * pos_proj_div.xyz + 0.5;
    SV_Target3.w = pos_proj_div.w; // no change
}