#version 460 core

// tell tessellator generates the triangles in CCW order
layout (triangles, equal_spacing, ccw) in;

layout (location = 0) in vec3 tscPos[];

uniform mat4 view_mat;
uniform mat4 proj_mat;

// we use gl_TessCoord (barycentric coordinate) to interpolate within a triangle (TCS output patch)
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main () {
    gl_Position = proj_mat * view_mat * vec4(interpolate3D(tscPos[0], tscPos[1], tscPos[2]), 1.0);
}