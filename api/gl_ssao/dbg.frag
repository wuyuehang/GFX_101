#version 460 core

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0_POS_VIEWSPACE;
uniform mat4 proj_mat;
#define K 64
uniform vec3 grandom[K];

void main() {
    vec3 Pos_viewspace = texture(TEX0_POS_VIEWSPACE, UV).rgb; // mesh.xyzw

    float AO = 0.0;
    for(int i = 0; i < K; i++) {
        vec4 Sample_viewspace = vec4(Pos_viewspace + grandom[i], 1.0);// spawn a randowm sample near the mesh.
        vec4 Sample_clipspace = proj_mat * Sample_viewspace; // project it from viewspace into clipspace
        vec2 Sample_uv = Sample_clipspace.xy;
        Sample_uv = Sample_uv.xy / Sample_clipspace.w;
        Sample_uv = 0.5 * Sample_uv + 0.5;

        float sampleDepth = texture(TEX0_POS_VIEWSPACE, Sample_uv).z; // along the same direction, the reference depth

        if (Sample_viewspace.z < sampleDepth) {
            //
        } else {
            AO += 1.0; // occluded by itself
        }
    }
    AO = 1.0 - AO / K;
    SV_Target = vec4(pow(AO, 2.0));
}