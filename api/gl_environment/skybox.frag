#version 460 core

layout (location = 0) in vec3 fUV;
layout (location = 0) out vec4 SV_Target;

uniform samplerCube Environment;

void main() {
    SV_Target = texture(Environment, fUV);
}
