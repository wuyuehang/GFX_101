#version 460 core

layout (location = 0) in vec3 vNor;
layout (location = 0) out vec4 SV_Target;

void main()
{
	SV_Target = vec4(vNor, 1.0);
}
