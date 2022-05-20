#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_DIFFUSE;
uniform float uGaussianWeights[5];

void main() {
    ivec2 resinfo = textureSize(TEX0_DIFFUSE, 0);
    float xsnap = 1.0 / float(resinfo.x);

      vec4 s4 = texture(TEX0_DIFFUSE, UV + vec2(4.0*xsnap, 0.0));
      vec4 s3 = texture(TEX0_DIFFUSE, UV + vec2(3.0*xsnap, 0.0));
      vec4 s2 = texture(TEX0_DIFFUSE, UV + vec2(2.0*xsnap, 0.0));
      vec4 s1 = texture(TEX0_DIFFUSE, UV + vec2(1.0*xsnap, 0.0));
      vec4 s0 = texture(TEX0_DIFFUSE, UV);
    vec4 s_m1 = texture(TEX0_DIFFUSE, UV - vec2(1.0*xsnap, 0.0));
    vec4 s_m2 = texture(TEX0_DIFFUSE, UV - vec2(2.0*xsnap, 0.0));
    vec4 s_m3 = texture(TEX0_DIFFUSE, UV - vec2(3.0*xsnap, 0.0));
    vec4 s_m4 = texture(TEX0_DIFFUSE, UV - vec2(4.0*xsnap, 0.0));

#if 1
    SV_Target = uGaussianWeights[4]*s4 + \
        uGaussianWeights[3]*s3 + \
        uGaussianWeights[2]*s2 + \
        uGaussianWeights[1]*s1 + \
        uGaussianWeights[0]*s0 + \
        uGaussianWeights[1]*s_m1 + \
        uGaussianWeights[2]*s_m2 + \
        uGaussianWeights[3]*s_m3 + \
        uGaussianWeights[4]*s_m4;
#else
    SV_Target = s0;
#endif
}
