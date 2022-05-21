#version 460 core

layout (location = 0) in vec3 Pos;
layout (location = 1) in vec3 Nor;

layout (location = 0) out vec4 SV_Target;

uniform vec3 light_viewspace;

void main() {
    // normalize normal
    vec3 N_dir = normalize(Nor);
    vec3 L_dir = normalize(light_viewspace - Pos);
    vec3 V_dir = normalize(0.0 - Pos);

    // diffuse
    float diffuse = max(dot(N_dir, L_dir), 0);
    SV_Target = vec4(diffuse);

    // specular
    vec3 halfway = normalize(V_dir + L_dir);
    float specular = pow(max(dot(halfway, N_dir), 0.0f), 16);
    SV_Target += vec4(specular);
}
