#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_ORIGIN;
uniform sampler2D TEX1_BLUR;
uniform float bloom_degree;

void main() {
    vec2 flipUV = vec2(UV.x, 1.0 - UV.y);
    SV_Target = texture(TEX0_ORIGIN, flipUV) + bloom_degree * texture(TEX1_BLUR, flipUV);
}
