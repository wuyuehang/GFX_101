#version 320 es
precision highp float;

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 SV_Target;

uniform sampler2D TEX0;

void main()
{
	SV_Target = texture(TEX0, UV);
}
