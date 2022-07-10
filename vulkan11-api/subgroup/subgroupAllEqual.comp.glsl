#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_vote : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
	// true if all invocations have same value T
	int T = dst.d[gl_SubgroupInvocationID];

	if (subgroupAllEqual(T)) {
		dst.d[gl_SubgroupInvocationID] = int(gl_SubgroupInvocationID);
	}
}
