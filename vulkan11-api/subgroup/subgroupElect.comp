#version 460
#extension GL_KHR_shader_subgroup_basic : require

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) writeonly buffer _dst {
    int d[];
} dst;

void main() {
    // Pick one active invocation, always the one with lowest gl_SubgroupInvocationID
    // Used for executing work on only one invocation
	if (subgroupElect()) {
		dst.d[gl_SubgroupInvocationID] = int(gl_SubgroupInvocationID);
	}
}
