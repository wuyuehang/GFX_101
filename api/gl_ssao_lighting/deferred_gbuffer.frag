#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec2 UV;

layout (location = 2) in vec3 Tan_viewspace;
layout (location = 3) in vec3 Bta_viewspace;
layout (location = 4) in vec3 Nor_viewspace;

layout (location = 0) out vec4 SV0_Pos_viewspace;
layout (location = 1) out vec4 SV1_Albedo;
layout (location = 2) out vec3 SV2_Normal_viewspace;
layout (location = 3) out vec4 SV3_AO;

uniform sampler2D TEX0_ALBEDO;
uniform sampler2D TEX3_NORMAL;
uniform sampler2D TEX4_AO;

void main() {
    SV0_Pos_viewspace = vec4(Pos_viewspace, 1.0);

    SV1_Albedo = texture(TEX0_ALBEDO, UV);

    vec3 N_tangentspace = texture(TEX3_NORMAL, UV).rgb;
    N_tangentspace = 2.0 * N_tangentspace - 1.0; // map to [-1, 1]
    N_tangentspace = normalize(N_tangentspace);

    // TBN (transform tangent space to view-model space)
    mat3 TBN = mat3(
        Tan_viewspace, // first column
        Bta_viewspace, // second column
        Nor_viewspace // third column
    );
    SV2_Normal_viewspace = TBN * N_tangentspace;

    SV3_AO = texture(TEX4_AO, UV);
}