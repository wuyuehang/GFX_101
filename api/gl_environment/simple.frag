#version 460 core

layout (location = 0) in vec3 vout_Pos; // position in view space
layout (location = 1) in vec3 vout_Nor; // normal in view space

layout (location = 0) out vec4 SV_Target;

uniform vec3 light_loc; // light in view space
uniform float roughness;

void main()
{
    // normalize normal
    vec3 N_dir = normalize(vout_Nor);

    // diffuse shading equation
    vec3 L_dir = normalize(light_loc - vout_Pos);
    float diffuse = max(dot(L_dir, N_dir), 0.0);
    SV_Target = vec4(diffuse);

    // specular shading equation
    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize(vec3(0.0) - vout_Pos);
    vec3 reflect_dir = normalize(reflect(-L_dir, vout_Nor));
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0f), roughness);
    SV_Target += vec4(specular);
}
