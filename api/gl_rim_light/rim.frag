#version 460 core

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNor;
layout (location = 2) in vec2 fUV;

layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;

void main() {
    vec3 N = normalize(fNor);
    vec3 view_dir = normalize(vec3(0.0) - fPos);
    float rim_factor = 1.0 - dot(N, view_dir);

    rim_factor = pow(rim_factor, 2);
    SV_Target = rim_factor * texture(TEX0_DIFFUSE, fUV);
}
