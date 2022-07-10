#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_shuffle : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
    // initially, T is equal to gl_SubgroupInvocationID
	int T = int(gl_SubgroupInvocationID);
    // shuffle head with tail, aka reverse
    T = subgroupShuffle(T, 31 - gl_SubgroupInvocationID);
	dst.d[gl_SubgroupInvocationID] = T;
}
