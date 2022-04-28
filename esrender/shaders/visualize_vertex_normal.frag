#version 320 es
precision highp float;

layout(location = 0) out vec4 SV_Target;

void main()
{
	SV_Target = vec4(1.0, 1.0, 0.0, 1.0);
}
