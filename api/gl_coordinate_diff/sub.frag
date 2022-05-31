#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out float SV_Target;

uniform sampler2D TEX0_DEPTH_REF;
uniform sampler2D TEX1_DEPTH_MANUAL;

void main() {
    float ref = texture(TEX0_DEPTH_REF, UV).r;
    float manual = texture(TEX1_DEPTH_MANUAL, UV).b; // Z componet

    SV_Target = (ref - manual);
}
