#version 320 es
precision highp float;

layout (location = 0) in vec2 fUV;

layout (location = 0) out vec4 SV_Target0; // albeto (diffuse)
layout (location = 1) out vec4 SV_Target1; // roughness
layout (location = 2) out vec4 SV_Target2; // normal

uniform sampler2D TEX0_DIFFUSE;
uniform sampler2D TEX2_ROUGHNESS; // align the index with Mesh.cpp
uniform sampler2D TEX3_NORMAL;

void main() {
    SV_Target0 = texture(TEX0_DIFFUSE, fUV);
    SV_Target1 = texture(TEX2_ROUGHNESS, fUV);
    SV_Target2 = texture(TEX3_NORMAL, fUV);
}
