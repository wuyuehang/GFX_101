#version 460 core

layout (location = 0) in vec3 vout_Pos; // position in view space
layout (location = 1) in vec3 vout_Nor; // normal in view space

layout (location = 0) out vec4 SV_Target;

uniform samplerCube Environment;

void main()
{
    // normalize normal
    vec3 N_dir = normalize(vout_Nor);

    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize(vout_Pos - vec3(0.0));
    vec3 reflect_dir = normalize(reflect(view_dir, vout_Nor)); // generate UV to sample the cubemap

    SV_Target = texture(Environment, reflect_dir);
}
