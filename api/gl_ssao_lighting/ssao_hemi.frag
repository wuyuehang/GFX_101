#version 460 core

layout (location = 0) in vec2 UV;

layout (location = 0) out vec4 SV_Target;
#define K 64
uniform vec3 ssaoKernel[K]; // orient +Z in tangent space
uniform float radius;
uniform mat4 proj_mat;
uniform sampler2D TEX1_POS_VIEWSPACE;
uniform sampler2D TEX3_NORMAL_VIEWSPACE;
uniform sampler2D TEX6_SSAO_NOISE;
#define NOISE_W 4.0
#define W 1024.0
#define bias 0.025

void main() {
    vec3 Pos_viewspace = texture(TEX1_POS_VIEWSPACE, UV).xyz;

    vec2 randomVecUV = UV * W / NOISE_W; // make repeat mode
    vec3 randomVec = texture(TEX6_SSAO_NOISE, randomVecUV).xyz;
    // Gramm-Schmidt process
    vec3 N = texture(TEX3_NORMAL_VIEWSPACE, UV).xyz; // lazy here, we actually need it in tangent space!!!
    vec3 Tangent = normalize(randomVec - N * dot(randomVec, N));
    vec3 Bitangent = cross(N, Tangent);
    mat3 TBN = mat3(Tangent, Bitangent, N);

    float AO = 0.0;
    for (int i = 0; i < K; i++) {
        vec3 samplePos = Pos_viewspace + TBN * (ssaoKernel[i] * radius); // bias a hemisphere
        vec4 offset = vec4(samplePos, 1.0);
        offset = proj_mat * offset;
        offset.xy /= offset.w;
        offset.xy = 0.5 * offset.xy + 0.5;
        float sampleDepth = texture(TEX1_POS_VIEWSPACE, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(Pos_viewspace.z - sampleDepth));
        AO += ((sampleDepth >= samplePos.z + bias) ? 1.0 : 0.0) * rangeCheck;
    }
    AO = 1.0 - AO / K;
    SV_Target = vec4(AO);
}
