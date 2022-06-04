#version 460 core

layout (location = 0) in vec2 UV;

layout (location = 0) out vec4 SV_Target;
#define K 64
uniform vec3 ssaoKernel[K];
uniform float radius;
uniform mat4 proj_mat;
uniform sampler2D TEX1_POS_VIEWSPACE;
#define bias 0.025

void main() {
    vec3 Pos_viewspace = texture(TEX1_POS_VIEWSPACE, UV).xyz;

    float AO = 0.0;
    for (int i = 0; i < K; i++) {
        vec3 samplePos = Pos_viewspace + ssaoKernel[i]*radius; // bias a sphere
        vec4 offset = vec4(samplePos, 1.0);
        offset = proj_mat * offset;
        offset.xy /= offset.w;
        offset.xy = 0.5 * offset.xy + 0.5;
        // in clip space, track along the projected direction to see the reference depth by visible surface
        float sampleDepth = texture(TEX1_POS_VIEWSPACE, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(Pos_viewspace.z - sampleDepth));
        AO += ((sampleDepth >= samplePos.z + bias) ? 1.0 : 0.0) * rangeCheck;
    }
    AO = 1.0 - AO / K;
    SV_Target = vec4(AO);
}
