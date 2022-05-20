#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;
uniform float threshold;

void main() {
    vec4 raw = texture(TEX0_DIFFUSE, UV);
    // Grayscale = 0.299R + 0.587G + 0.114B
    float grayscale = dot(vec3(0.299, 0.587, 0.114), raw.rgb);

    if (grayscale < threshold) {
        SV_Target = vec4(0.0);
    } else {
        SV_Target = vec4(1.0);
    }
}