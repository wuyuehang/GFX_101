#version 320 es
precision highp float;

layout (location = 0) in vec3 vNor;
layout (location = 0) out vec4 SV_Target;

void main()
{
	SV_Target = vec4(0.5 * normalize(vNor) + vec3(0.5), 1.0);
}
