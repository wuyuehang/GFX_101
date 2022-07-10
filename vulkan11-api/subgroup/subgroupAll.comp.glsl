#version 460
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_vote : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) buffer _dst {
    int d[];
} dst;

void main() {
	// true if all invocations have d[0] == 66666666u
	bool cond = (dst.d[0] == 66666666u);

	if (subgroupAll(cond)) {
		dst.d[gl_SubgroupInvocationID] = int(gl_SubgroupInvocationID);
	}
}
