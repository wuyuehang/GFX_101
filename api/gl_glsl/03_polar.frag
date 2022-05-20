#version 460 core

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

#define AMBIENT_R (0.25+0.25*sin(uTime))
#define AMBIENT_G (0.25+0.25*sin(2.0*uTime))
#define AMBIENT_B (0.25+0.25*sin(4.0*uTime))

#define T0 0.02
#define T1 0.01

#define NUM 10

mat2 RotationBy(float r) {
    mat2 rotation = {
        vec2(cos(r), -sin(r)),
        vec2(sin(r), cos(r)),
    };
    return rotation;
}

void main() {
    vec2 pos = gl_FragCoord.xy / uResInfo.xy;

    pos = 2.0*pos-1.0;

    pos = RotationBy(uTime)*pos;

    fColor = vec4(AMBIENT_R, AMBIENT_G, AMBIENT_B, 0.0);

    float radius;
    float angle;
    float fc;
    float w[2];
    float split_amplifier;
    float split;
    int i;

    for (i = 1; i <= NUM; i++) {
        radius = distance(vec2(0.0), vec2(pos.x, pos.y));
        angle = atan(pos.x, pos.y);

        split_amplifier = 7.0;
        split = split_amplifier*(1.0+sin(uTime));
        fc = abs(cos(split*angle));
        fc *= 0.5*(1.0+sin(uTime));

        fc *= float(i)/float(NUM);

        w[0] = 1.0-smoothstep(fc, fc+T1, radius);
        w[1] = 1.0-smoothstep(fc+T0, fc+T1+T0, radius);

        fc = w[1]-w[0];

        fColor += vec4(fc, fc, 0.0, 0.0);
    }

    fColor *= 0.5;
}
