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
    // our object's depth if transform in light's clipspace
    float D_obj = projUV.z;
    // shadow acne
    float bias = max(0.05 * (1.0 - dot(N_dir, L_dir)), 0.005);

    float shadow = 0.0;
    vec2 snap = 1.0 / textureSize(TEX0_SHADOWMAP, 0);
    int radius = 3;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            // .xy can be used as coordinate to texture the shadow map
            // slightly snap
            float pcf_D_referece = texture(TEX0_SHADOWMAP, projUV.xy + vec2(i, j)*snap).r;
            if ((D_obj - bias) > pcf_D_referece) {
                shadow += 1.0;
            }
        }
    }
    shadow = shadow / float(radius*radius); // average

    if (D_obj > 1.0) {
        // object is outside NDC.z (far plane), we're not in the shadow
        SV_Target = ADS;
    } else {
        SV_Target = ADS * (1.0 - shadow);
    }
}
