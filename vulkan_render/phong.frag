#version 460

layout(location = 0) in vec3 vout_Pos; // position in view space
layout(location = 1) in vec3 vout_Nor; // normal in view space

layout(location = 0) out vec4 SV_Target;

layout(set = 0, binding = 1) uniform UBO {
    vec3 view_loc; // location in view space
} LIGHT;

void main()
{
    // diffuse
    vec3 light_dir = normalize(LIGHT.view_loc - vout_Pos);
    float diffuse = max(dot(light_dir, vout_Nor), 0.0);

    // specular
    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize( - vout_Pos);
    vec3 reflect_dir = reflect(light_dir, vout_Nor);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0f), 128);

    SV_Target = vec4(specular + diffuse);
}
