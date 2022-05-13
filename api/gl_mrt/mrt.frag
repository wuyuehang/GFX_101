#version 460 core

layout (location = 0) in vec2 fUV;
layout (location = 0) out vec4 SV_Target0; // albeto (diffuse)
layout (location = 1) out vec4 SV_Target1; // roughness

uniform sampler2D TEX0_DIFFUSE;
uniform sampler2D TEX1_SPECULAR; // unused
uniform sampler2D TEX2_ROUGHNESS; // align the index with Mesh.cpp

void main() {
    SV_Target0 = texture(TEX0_DIFFUSE, fUV);
    SV_Target1 = texture(TEX2_ROUGHNESS, fUV);
}
