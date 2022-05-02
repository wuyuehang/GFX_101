#version 460 core

layout (location = 0) in vec3 vout_Pos; // position in view space
layout (location = 1) in vec3 vout_Nor; // normal in view space
layout (location = 2) in vec2 vout_UV;

layout (location = 0) out vec4 SV_Target;

uniform vec3 light_loc[2]; // light in view space
//uniform vec3 La;
//uniform vec3 Ka;
//uniform vec3 Ld; // light_intensity;
//uniform vec3 Kd; // diffuse reflectivity
//uniform vec3 Ls;
//uniform vec3 Ks;
//uniform float roughness;

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
uniform sampler2D TEX2_ROUGHNESS; // pbr roughness texture

vec3 ads(vec3 light, vec3 pos, vec3 nor, vec2 uv, Material material, Attenuation attenuation) {
    vec3 ADS;

    // normalize normal
    vec3 N_dir = normalize(nor);

    // diffuse shading equation
    vec3 L_dir = normalize(light - pos);
    float diffuse = max(dot(L_dir, N_dir), 0.0);

    ADS = vec3(diffuse) * material.Kd * texture(TEX0_DIFFUSE, uv).rgb;

    // specular shading equation
    // since we're in view coordinate space, the view is origin at 0
    vec3 view_dir = normalize(vec3(0.0) - pos);
    vec3 reflect_dir = normalize(reflect(-L_dir, nor));
    //
    vec3 specular;
    specular.r = pow(max(dot(view_dir, reflect_dir), 0.0f), texture(TEX2_ROUGHNESS, uv).r);
    specular.g = pow(max(dot(view_dir, reflect_dir), 0.0f), texture(TEX2_ROUGHNESS, uv).g);
    specular.b = pow(max(dot(view_dir, reflect_dir), 0.0f), texture(TEX2_ROUGHNESS, uv).b);

    ADS += vec3(specular) * material.Ks * texture(TEX1_SPECULAR, uv).rgb;

    // Calculate attenuation
    float distance = length(light - pos);
    float f_attenuation = 1.0 / (attenuation.Kc + attenuation.Kl * distance + attenuation.Kq * distance * distance);
    ADS *= vec3(f_attenuation);

    return ADS;
}

void main()
{
    SV_Target = vec4(ads(light_loc[0], vout_Pos, vout_Nor, vout_UV, material, attenuation), 1.0);
    //SV_Target += vec4(ads(light_loc[1], vout_Pos, vout_Nor, vout_UV, material, attenuation), 1.0);
}
