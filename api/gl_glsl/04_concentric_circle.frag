#version 460 core

#define PI 3.14159265758

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

#define R_SPEED 0.5
#define G_SPEED 1.0
#define B_SPEED 2.0

#define AMBIENT_R (0.15+0.15*sin(uTime))
#define AMBIENT_G (0.15+0.15*sin(2.0*uTime))
#define AMBIENT_B (0.15+0.15*sin(4.0*uTime))

#define ENA_R 1
#define ENA_G 1
#define ENA_B 1

mat2 RotationBy(float r) {
    mat2 rotation = {
        vec2(cos(r), -sin(r)),
        vec2(sin(r), cos(r)),
    };
    return rotation;
}

void main() {
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;

    /* remap coordinate origin */
    x = 2.0*x-1.0;
    y = 2.0*y-1.0;

    fColor = vec4(AMBIENT_R, AMBIENT_G, AMBIENT_B, 0.0);
    float radius;
    float fc;
    float gap_amplifier;
    float control_point;
    float illumi_amplifier;

    /* RED CONCENTRIC */
    #if ENA_R
        /* move concentric origin */
        control_point = 0.5+0.5*cos(uTime);

        /* distance field magic */
        fc = distance(vec2(control_point), abs(RotationBy(R_SPEED*uTime)*vec2(x, y)));

        /* tune the gap between the neighbour foldings */
        gap_amplifier = 9.0+7.0*sin(uTime);
        fc *= gap_amplifier;

        /* dark magic */
        fc = fract(fc);

        /* tune the final illumination */
        illumi_amplifier = 0.75+0.25*cos(2.0*uTime);
        fc *= illumi_amplifier;

        fColor += vec4(fc, vec3(0.0));
    #endif

    /* GREEN CONCENTRIC */
    #if ENA_G
        /* move concentric origin */
        control_point = 0.5+0.5*cos(uTime);

        /* distance field magic */
        fc = distance(vec2(control_point), abs(RotationBy(G_SPEED*uTime)*vec2(x, y)));

        /* tune the gap between the neighbour foldings */
        gap_amplifier = 9.0+7.0*sin(uTime);
        fc *= gap_amplifier;

        /* dark magic */
        fc = fract(fc);

        /* tune the final illumination */
        illumi_amplifier = 0.75+0.25*cos(2.0*uTime);
        fc *= illumi_amplifier;

        fColor += vec4(0.0, fc, vec2(0.0));
    #endif

    /* BLUE CONCENTRIC */
    #if ENA_B
        /* move concentric origin */
        control_point = 0.5+0.5*cos(uTime);

        /* distance field magic */
        fc = distance(vec2(control_point), abs(RotationBy(B_SPEED*uTime)*vec2(x, y)));

        /* tune the gap between the neighbour foldings */
        gap_amplifier = 9.0+7.0*sin(uTime);
        fc *= gap_amplifier;

        /* dark magic */
        fc = fract(fc);

        /* tune the final illumination */
        illumi_amplifier = 0.75+0.25*cos(2.0*uTime);
        fc *= illumi_amplifier;

        fColor += vec4(0.0, 0.0, fc, 0.0);
    #endif
}
