#version 460

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ipa;
layout(location = 0) out vec4 SV_Target;

void main() {
    SV_Target = subpassLoad(ipa);
}
