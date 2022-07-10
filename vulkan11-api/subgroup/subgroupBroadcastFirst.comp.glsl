#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
    // initially, T is equal to gl_SubgroupInvocationID
	int T = int(gl_SubgroupInvocationID);
    // only invocation with lowest active gl_SubgroupInvocationID can broadcast the T
    // in this case, T will be 0.
    // now all invocations see T = 0
    T = subgroupBroadcastFirst(T);
	dst.d[gl_SubgroupInvocationID] = T;
}
