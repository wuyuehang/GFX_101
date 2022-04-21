#version 460
#extension GL_EXT_geometry_shader : require

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 vout_Nor[];

layout(set = 0, binding = 0) uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
} MVP;

#define MAG 0.02f

void main() {
    gl_Position = MVP.proj * gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = MVP.proj * (gl_in[0].gl_Position + MAG * vec4(vout_Nor[0], 0.0));
    EmitVertex();
    EndPrimitive();

    gl_Position = MVP.proj * gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = MVP.proj * (gl_in[1].gl_Position + MAG * vec4(vout_Nor[1], 0.0));
    EmitVertex();
    EndPrimitive();

    gl_Position = MVP.proj * gl_in[2].gl_Position;
    EmitVertex();
    gl_Position = MVP.proj * (gl_in[2].gl_Position + MAG * vec4(vout_Nor[2], 0.0));
    EmitVertex();
    EndPrimitive();
}