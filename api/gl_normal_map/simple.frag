#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec3 Nor_viewspace;
layout (location = 2) in vec2 fUV;
layout (location = 3) in vec3 LightDir_viewspace;

layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;

void main() {
    // Material
    vec4 materialBaseColor = texture(TEX0_DIFFUSE, fUV);

    vec3 N = normalize(Nor_viewspace);

    // diffuse shading equation
    vec3 L = normalize(LightDir_viewspace);
    float diffuse = max(0.0, dot(L, N));

    SV_Target = vec4(diffuse) * materialBaseColor;
}
