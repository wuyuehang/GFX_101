#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_arithmetic : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
    // return the summation of all active invocations' T
    // 0 + 1 + 2 + ... + 31 = 496
    int T = int(gl_SubgroupInvocationID);
	dst.d[gl_SubgroupInvocationID] = subgroupAdd(T);
}
