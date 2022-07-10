#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_arithmetic : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
    // return exclusive scan summation of all active invocations' T
    // invocation[0] = 0
    // invocation[1] = gl_SubgroupInvocationID (0) = 0
    // invocation[2] = gl_SubgroupInvocationID (0) + gl_SubgroupInvocationID (1) = 1
    // invocation[3] = gl_SubgroupInvocationID (0) + gl_SubgroupInvocationID (1) + gl_SubgroupInvocationID (2) = 3
    // ...
    // invocation[30] = gl_SubgroupInvocationID (0) + ... + gl_SubgroupInvocationID (29) = 435
    // invocation[31] = gl_SubgroupInvocationID (0) + ... + gl_SubgroupInvocationID (30) = 465
    int T = int(gl_SubgroupInvocationID);
	dst.d[gl_SubgroupInvocationID] = subgroupExclusiveAdd(T);
}