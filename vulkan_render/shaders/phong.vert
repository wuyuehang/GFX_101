#version 460

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNor;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec3 vout_Pos;
layout(location = 1) out vec3 vout_Nor;

layout(set = 0, binding = 0) uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
} MVP;

void main() {
	gl_Position = MVP.proj * MVP.view * MVP.model * vec4(vPos, 1.0);

    // calculate light equation in model-view coordinate
    vout_Pos = vec3(MVP.view * MVP.model * vec4(vPos, 1.0));
    vout_Nor = mat3(transpose(inverse(MVP.view * MVP.model))) * vNor;
}
