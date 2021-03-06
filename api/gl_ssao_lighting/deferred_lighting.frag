#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_ALBEDO;
uniform sampler2D TEX1_POS_VIEWSPACE;
uniform sampler2D TEX3_NORMAL_VIEWSPACE;
uniform sampler2D TEX4_AO; // either from original model or generated by us
uniform vec3 LightPos_viewspace;

void main() {
    // Material
    vec4 materialBaseColor = texture(TEX0_ALBEDO, UV);
    float ao_factor = texture(TEX4_AO, UV).r;

    //
    vec3 Pos_viewspace = texture(TEX1_POS_VIEWSPACE, UV).xyz;
    vec3 LightDir_viewspace = LightPos_viewspace - Pos_viewspace;
    vec3 L_viewspace = normalize(LightDir_viewspace);

    // Bump map
    vec3 N_viewspace = texture(TEX3_NORMAL_VIEWSPACE, UV).rgb;

    // diffuse shading equation
    float diffuse = max(0.0, dot(L_viewspace, N_viewspace));

    SV_Target = vec4(diffuse) * materialBaseColor * ao_factor;
}