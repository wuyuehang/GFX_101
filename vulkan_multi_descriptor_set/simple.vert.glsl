#version 460

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vCoordinate;

layout(location = 0) out vec2 fCoordinate;

void main() {
	gl_Position = vPosition;
	fCoordinate = vCoordinate;
}
