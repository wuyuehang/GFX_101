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
    // only gl_SubgroupInvocationID == 31 can broadcast the T
    // now all invocations see T = 31
    T = subgroupBroadcast(T, 31); // ID must be constant
	dst.d[gl_SubgroupInvocationID] = T;
}
