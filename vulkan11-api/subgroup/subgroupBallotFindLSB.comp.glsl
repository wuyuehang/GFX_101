#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
    // even invocation ballot 1, odd invocation ballot 0
    bool cond = !bool(gl_SubgroupInvocationID % 2);

    uvec4 T = subgroupBallot(cond);
    // now T expects (0x55555555, 0x0, 0x0, 0x0)

	dst.d[gl_SubgroupInvocationID] = int(subgroupBallotFindLSB(T));
    // dst.d expects 0;
}
