#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_clustered : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
	// T is 0, 1, 2, ..., 31
    int T = int(gl_SubgroupInvocationID);

	dst.d[gl_SubgroupInvocationID] = subgroupClusteredAdd(T, 4);
    // dst.d expects 6, 6, 6, 6, 22, 22, 22, 22, ..., 118, 118, 118, 118;
}
