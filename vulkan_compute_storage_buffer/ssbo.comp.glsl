#version 460

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
layout (set = 0, binding = 0) writeonly buffer _dst {
    int d[];
} dst;

void main() {
    dst.d[gl_LocalInvocationID.x] = 0x01020304;
}