#version 460

layout(location = 0) in vec3 fNor;
layout(location = 0) out vec4 SV_Target;

void main()
{
	SV_Target = vec4(fNor, 1.0);
}
