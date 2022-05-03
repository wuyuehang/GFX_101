#version 320 es
precision highp float;

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

struct Material {
    //vec3 Ka; // ambient
    vec3 Kd; // diffuse
    vec3 Ks; // specular
};

#define TOON_LEVEL 8.0

uniform Material material;

uniform sampler2D TEX0_DIFFUSE;  // diffuse texture

vec3 toon(vec3 light, vec3 pos, vec3 nor, vec2 uv, Material material) {
    vec3 TOON;

    // normalize normal
    vec3 N_dir = normalize(nor);

    // diffuse shading equation
    vec3 L_dir = normalize(light - pos);
    float diffuse = max(dot(L_dir, N_dir), 0.0);

    TOON = vec3(diffuse) * material.Kd * texture(TEX0_DIFFUSE, uv).rgb;
    TOON = floor(TOON * TOON_LEVEL) / TOON_LEVEL;

    return TOON;
}

void main()
{
    SV_Target = vec4(toon(light_loc[0], vout_Pos, vout_Nor, vout_UV, material), 1.0);
    SV_Target += vec4(toon(light_loc[1], vout_Pos, vout_Nor, vout_UV, material), 1.0);
}
