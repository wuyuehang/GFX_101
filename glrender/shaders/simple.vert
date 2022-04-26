#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;

layout (location = 0) out vec3 fNor;

uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
} MVP; // named uniform block

void main() {
	gl_Position = MVP.proj * MVP.view * MVP.model * vec4(vPos, 1.0);
	fNor = vNor;
}
