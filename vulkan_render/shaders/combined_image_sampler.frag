#version 460

layout(location = 0) in vec2 fUV;
layout(location = 0) out vec4 SV_Target;

layout(set = 0, binding = 1) uniform sampler2D tex;

void main()
{
	SV_Target = texture(tex, fUV);
}
