#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec3 Nor_viewspace;
layout (location = 2) in vec2 UV;

layout (location = 0) out vec4 SV_Target;

uniform vec3 LightPos_viewspace;
uniform sampler2D TEX0_DIFFUSE;

void main() {
    // Material
    vec4 materialBaseColor = 0.125 * texture(TEX0_DIFFUSE, UV);
    // vec3 N = normalize(Nor_viewspace);
    // vec3 L = normalize(LightPos_viewspace - Pos_viewspace);

    // diffuse shading equation
    // float diffuse = max(0.0, dot(L, N)); // not used here

    // point light
    float dist = length(LightPos_viewspace - Pos_viewspace);
    vec4 L_intensity = vec4(1.0, 1.0, 0.0, 1.0)  / (1.0 + 0.7 * dist + 1.8 * dist * dist);
    SV_Target = L_intensity + materialBaseColor;

}