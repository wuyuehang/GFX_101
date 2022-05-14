#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 UV[];
layout (location = 0) out vec2 fUV;

layout (binding = 1, offset = 0) uniform atomic_uint acPrim;

uniform mat4 proj_mat;

void main() {
    fUV = UV[0];
    gl_Position = proj_mat * gl_in[0].gl_Position;
    EmitVertex();

    fUV = UV[1];
    gl_Position = proj_mat * gl_in[1].gl_Position;
    EmitVertex();

    fUV = UV[2];
    gl_Position = proj_mat * gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
    atomicCounterIncrement(acPrim);
}
