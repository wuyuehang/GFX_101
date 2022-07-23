#version 460

layout(location = 0) in vec4 vPos;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 fColor;

void main() {
	gl_Position = vPos;
	fColor = vColor;
}
