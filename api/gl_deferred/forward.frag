#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 LightDir_tangentspace;

layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;
uniform sampler2D TEX3_NORMAL;

void main() {
    // Material
    vec4 materialBaseColor = texture(TEX0_DIFFUSE, UV);

    // fetch normal map
    vec3 N_tangentspace = texture(TEX3_NORMAL, UV).rgb;
    N_tangentspace = 2.0 * N_tangentspace - 1.0; // map to [-1, 1]
    N_tangentspace = normalize(N_tangentspace);

    // diffuse shading equation
    vec3 L_tangentspace = normalize(LightDir_tangentspace);
    float diffuse = max(0.0, dot(L_tangentspace, N_tangentspace));

    SV_Target = vec4(diffuse) * materialBaseColor;
}
