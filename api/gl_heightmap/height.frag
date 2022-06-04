#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 LightDir_tangentspace;
layout (location = 3) in vec3 ViewDir_tangentspace;

layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;
uniform sampler2D TEX3_NORMAL;
uniform sampler2D TEX7_HEIGHT;

void main() {
    //
#if 0
    vec3 V = vec3(0.0) - Pos_viewspace;
#else
    vec3 V = ViewDir_tangentspace;
#endif
    V = normalize(V);

    float height = texture(TEX7_HEIGHT, UV).r;
    vec2 uv = UV + V.xy * height;

    // Material
    vec4 materialBaseColor = texture(TEX0_DIFFUSE, uv);

    // fetch normal map
    vec3 N_tangentspace = texture(TEX3_NORMAL, uv).rgb;
    N_tangentspace = 2.0 * N_tangentspace - 1.0; // map to [-1, 1]
    N_tangentspace = normalize(N_tangentspace);

    // diffuse shading equation
    vec3 L_tangentspace = normalize(LightDir_tangentspace);
    float diffuse = max(0.0, dot(L_tangentspace, N_tangentspace));

    SV_Target = vec4(diffuse) * materialBaseColor;
}
