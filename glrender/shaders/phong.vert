#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;

layout (location = 0) out vec3 vout_Pos;
layout (location = 1) out vec3 vout_Nor;

uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} MVP; // named uniform block

void main() {
    gl_Position = MVP.proj * MVP.view * MVP.model * vec4(vPos, 1.0);

    // calculate light equation in model-view coordinate
    vout_Pos = vec3(MVP.view * MVP.model * vec4(vPos, 1.0));
    vout_Nor = mat3(MVP.view * MVP.model) * vNor;
}
