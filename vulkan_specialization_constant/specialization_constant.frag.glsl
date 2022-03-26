#version 460

layout(location = 0) in vec2 fCoordinate;
layout(location = 0) out vec4 SV_Target;

layout(set = 0, binding = 0) uniform sampler2D tex;

layout (constant_id = 0) const bool const_a = false;
layout (constant_id = 1) const bool const_b = false;

void main()
{
	if (const_a && const_b) {
		SV_Target = texture(tex, fCoordinate);
	} else {
		SV_Target = vec4(0.0);
	}
}
