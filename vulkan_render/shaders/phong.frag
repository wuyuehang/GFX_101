#version 460

layout(location = 0) in vec3 vout_Pos; // position in view space
layout(location = 1) in vec3 vout_Nor; // normal in view space

layout(location = 0) out vec4 SV_Target;

layout(set = 0, binding = 1) uniform UBO {
    vec3 light_loc; // location in view space
    float roughness;
} LIGHT;

void main()
{
    // normalize normal
    vec3 N_dir = normalize(vout_Nor);

    // diffuse shading equation
    vec3 L_dir = normalize(LIGHT.light_loc - vout_Pos);
    float diffuse = max(dot(L_dir, N_dir), 0.0);
#if 0
    SV_Target = Ld * Kd * diffuse;
#else
    SV_Target = vec4(diffuse);
#endif

    // specular shading equation
    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize(vec3(0.0) - vout_Pos);
    vec3 reflect_dir = normalize(reflect(-L_dir, vout_Nor));
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0f), LIGHT.roughness);

    SV_Target += vec4(specular);
}
