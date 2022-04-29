#version 460 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 vout_Nor[];

uniform mat4 proj_mat;

#define MAG 0.04f

void main() {
    gl_Position = proj_mat * gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = proj_mat * (gl_in[0].gl_Position + MAG * vec4(vout_Nor[0], 0.0));
    EmitVertex();
    EndPrimitive();

    gl_Position = proj_mat * gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = proj_mat * (gl_in[1].gl_Position + MAG * vec4(vout_Nor[1], 0.0));
    EmitVertex();
    EndPrimitive();

    gl_Position = proj_mat * gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = proj_mat * (gl_in[2].gl_Position + MAG * vec4(vout_Nor[2], 0.0));
    EmitVertex();
    EndPrimitive();
}