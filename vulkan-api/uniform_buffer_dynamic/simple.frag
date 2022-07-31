#version 460

layout (location = 0) in vec4 fColor;
layout (location = 0) out vec4 SV_Target;

void main()
{
	SV_Target = fColor;
}
