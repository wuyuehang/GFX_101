#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec3 Nor_viewspace;
layout (location = 2) in vec2 UV;

layout (location = 0) out vec4 SV_Target;

uniform float SpotlightCutoff;
uniform vec3 SpotlightAttenuation;

uniform vec3 LightPos_viewspace;
uniform sampler2D TEX0_DIFFUSE;

void main() {
    // Material
    vec4 materialBaseColor = 0.125 * texture(TEX0_DIFFUSE, UV);
    // vec3 N = normalize(Nor_viewspace);
    // vec3 L = normalize(LightPos_viewspace - Pos_viewspace);

    // diffuse shading equation
    // float diffuse = max(0.0, dot(L, N)); // not used here

    // spotlight
    vec3 LToPixel = normalize(Pos_viewspace - LightPos_viewspace);
    float spot_factor = dot(LToPixel, SpotlightAttenuation);
    if (spot_factor > SpotlightCutoff) {
        float dist = length(LightPos_viewspace - Pos_viewspace);
        vec4 L_intensity = vec4(1.0, 1.0, 0.0, 1.0)  / (1.0 + 0.7 * dist + 1.8 * dist * dist);
        // interpolate spot_factor
        float interp = (spot_factor - SpotlightCutoff)/(1.0 - SpotlightCutoff);
        SV_Target = interp * L_intensity + materialBaseColor;
    } else {
        SV_Target = materialBaseColor;
    }
}
