#version 460 core

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

#define M (0.5+0.5*sin(2.5*uTime))
#define A (mod(abs(sin(uTime)), 2.0))

#define AMBIENT_R (0.15+0.15*sin(uTime))
#define AMBIENT_G (0.15+0.15*sin(2.0*uTime))
#define AMBIENT_B (0.15+0.15*sin(4.0*uTime))

#define INNER_COE 12
#define OUTTE_COE 2

void main() {
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;

    float radius[2];

    fColor = vec4(AMBIENT_R, AMBIENT_G, AMBIENT_B, 0.0);
    //fColor = vec4(0.2, 0.1, 0.2, 0.0);
    vec4 cc = vec4(0.0);

    /* horizontal */
    radius[0] = pow(distance(vec2(0.01, 0.5), vec2(x, y)), distance(vec2(A, 0.5), vec2(x, y)));
    radius[0] = pow(radius[0], INNER_COE);
    radius[1] = pow(radius[0], OUTTE_COE);

    cc = mix(vec4(radius[0], 0.0, 0.0, 0.0), vec4(0.0, radius[1], 0.0, 0.0), M);
    fColor += cc;

    radius[0] = pow(distance(vec2(0.01, 0.5), vec2(1.0-x, y)), distance(vec2(A, 0.5), vec2(1.0-x, y)));
    radius[0] = pow(radius[0], INNER_COE);
    radius[1] = pow(radius[0], OUTTE_COE);

    cc = mix(vec4(radius[0], 0.0, 0.0, 0.0), vec4(0.0, radius[1], 0.0, 0.0), M);
    fColor += cc;

    /* vertical */
    radius[0] = pow(distance(vec2(0.5, 0.01), vec2(x, y)), distance(vec2(0.5, A), vec2(x, y)));
    radius[0] = pow(radius[0], INNER_COE);
    radius[1] = pow(radius[0], OUTTE_COE);

    cc = mix(vec4(0.0, 0.0, radius[0], 0.0), vec4(radius[1], radius[1], 0.0, 0.0), M);
    fColor += cc;

    radius[0] = pow(distance(vec2(0.5, 0.01), vec2(x, 1.0-y)), distance(vec2(0.5, A), vec2(x, 1.0-y)));
    radius[0] = pow(radius[0], INNER_COE);
    radius[1] = pow(radius[0], OUTTE_COE);

    cc = mix(vec4(0.0, 0.0, radius[0], 0.0), vec4(radius[1], radius[1], 0.0, 0.0), M);
    fColor += cc;
}
