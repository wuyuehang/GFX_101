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

	SV_Target = vec4(diffuse);
}
