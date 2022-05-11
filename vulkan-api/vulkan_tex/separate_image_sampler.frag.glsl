#version 460

layout(location = 0) in vec2 fCoordinate;
layout(location = 0) out vec4 SV_Target;

layout(set = 0, binding = 0) uniform texture2D tex;
layout(set = 0, binding = 1) uniform sampler sam;

void main()
{
	SV_Target = texture(sampler2D(tex, sam), fCoordinate);
}
