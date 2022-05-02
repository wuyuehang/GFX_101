#version 460 core

layout (location = 0) in vec3 vout_Pos; // position in view space
layout (location = 1) in vec3 vout_Nor; // normal in view space
layout (location = 2) in vec2 vout_UV;

layout (location = 0) out vec4 SV_Target;

uniform vec3 light_loc; // light in view space
//uniform vec3 La;
//uniform vec3 Ka;
//uniform vec3 Ld; // light_intensity;
//uniform vec3 Kd; // diffuse reflectivity
//uniform vec3 Ls;
//uniform vec3 Ks;
uniform float roughness;

struct Material {
    //vec3 Ka; // ambient
    vec3 Kd; // diffuse
    vec3 Ks; // specular
};

struct Attenuation {
    float Kc;
    float Kl;
    float Kq;
};

uniform Material material;
uniform Attenuation attenuation;

uniform sampler2D TEX0_DIFFUSE;  // diffuse texture
uniform sampler2D TEX1_SPECULAR; // specular texture

void main()
{
    // normalize normal
    vec3 N_dir = normalize(vout_Nor);

    // diffuse shading equation
    vec3 L_dir = normalize(light_loc - vout_Pos);
    float diffuse = max(dot(L_dir, N_dir), 0.0);
#if 0
    SV_Target = Ld * Kd * diffuse;
#else
    SV_Target = vec4(diffuse) * vec4(material.Kd, 1.0) * texture(TEX0_DIFFUSE, vout_UV);
#endif

    // specular shading equation
    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize(vec3(0.0) - vout_Pos);
    vec3 reflect_dir = normalize(reflect(-L_dir, vout_Nor));
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0f), roughness);

    SV_Target += vec4(specular) * vec4(material.Ks, 1.0) * texture(TEX1_SPECULAR, vout_UV);

    // Calculate attenuation
    float distance = length(light_loc - vout_Pos);
    float f_attenuation = 1.0 / (attenuation.Kc + attenuation.Kl * distance + attenuation.Kq * distance * distance);
    SV_Target *= vec4(f_attenuation);
}
