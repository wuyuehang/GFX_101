#version 460 core

layout (location = 0) in vec3 Pos_viewspace;
layout (location = 1) in vec3 Nor_viewspace;
layout (location = 2) in vec4 Pos_lightclipspace;
layout (location = 0) out vec4 SV_Target;

uniform vec3 light_viewspace;
uniform sampler2D TEX0_SHADOWMAP;

vec4 calculate_ADS(vec3 N_dir, vec3 L_dir, vec3 V_dir) {
    vec4 ADS;
    // diffuse
    float diffuse = max(dot(N_dir, L_dir), 0);
    ADS = vec4(diffuse);
    // specular
    vec3 halfway = normalize(V_dir + L_dir);
    float specular = pow(max(dot(halfway, N_dir), 0.0f), 16);
    ADS += vec4(specular);

    return ADS;
}

void main() {
    vec3 N_dir = normalize(Nor_viewspace);
    vec3 L_dir = normalize(light_viewspace - Pos_viewspace);
    vec3 V_dir = normalize(0.0 - Pos_viewspace);

    vec4 ADS = calculate_ADS(N_dir, L_dir, V_dir);

    // calculate shadow
    // transform into NDC space
    vec3 projUV = Pos_lightclipspace.xyz / Pos_lightclipspace.w;
    // map NDC.xyz from [-1.0, 1.0] to [0.0, 1.0]
    projUV = projUV * 0.5 + 0.5;
    // .xy can be used as coordinate to texture the shadow map
    float D_nearest_referece = texture(TEX0_SHADOWMAP, projUV.xy).r;
    // our object's depth if transform in light's clipspace
    float D_obj = projUV.z;
    // shadow acne
    float bias = max(0.05 * (1.0 - dot(N_dir, L_dir)), 0.005);
    D_obj -= bias;

    if (D_obj > D_nearest_referece) {
        // we're in the shadow
        SV_Target = vec4(0.0);
    } else if (D_obj > 1.0) {
        // object is outside NDC.z (far plane), we're not in the shadow
        SV_Target = ADS;
    } else {
        SV_Target = ADS;
    }
}
