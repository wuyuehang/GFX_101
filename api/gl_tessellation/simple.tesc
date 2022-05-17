#version 460 core

// TCS take control patch as input
// for simplicity, we treat original triangle as control patch

// TCS out patch
layout (vertices = 3) out;

layout (location = 0) in vec3 vout_Pos[];
layout (location = 0) out vec3 tscPos[];

uniform float uOuterLevel;
uniform float uInnerLevel;

void main () {
    // passthrough
    tscPos[gl_InvocationID] = vout_Pos[gl_InvocationID];

    // TLs determine the Tessellation level of detail
    // how many triangles to generate for the patch

    // outer level determines the number of segment on each edge. Triangle patch needs 3
    gl_TessLevelOuter[0] = uOuterLevel;
    gl_TessLevelOuter[1] = uOuterLevel;
    gl_TessLevelOuter[2] = uOuterLevel;

    // inner level determines the how many rings the triangle domain contain
    gl_TessLevelInner[0] = uInnerLevel;
}