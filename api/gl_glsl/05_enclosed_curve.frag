#version 460 core

#define PI 3.141592657589793

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

void main() {
    float w[2];
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;
    float fc;
    x = 2.0*x-1.0;
    y = 2.0*y-1.0;

    // paint circle, sharp edge, green
    fc = abs(cos(uTime))*(x*x + y*y);
    // w[0] = step(0.05, fc);
    // w[1] = step(0.06, fc);
    w[0] = smoothstep(0.05, 0.07, fc);
    w[1] = smoothstep(0.06, 0.07, fc);
    fColor = vec4(w[0]-w[1], vec3(0.0));

    // paint ellipse, yellow
    fc = x*x*abs(sin(4.0*uTime)) + y*y*0.3;
    w[0] = smoothstep(0.04, 0.05, fc);
    w[1] = smoothstep(0.045, 0.05, fc);
    fColor += vec4(vec2(w[0]-w[1]), vec2(0.0));

    // paint hyperbola, white
    fc = x*x*0.5 - y*y*0.6;
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += vec4(w[0]-w[1]);

    // dynamic
    fc = x*x*sin(uTime) + y*y*cos(uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += vec4(0.0, vec3(w[0]-w[1]));

    fc = x*x*sin(5.0*uTime) + y*y*cos(5.0*uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += vec4(w[0]-w[1], 0.0, w[0]-w[1], 0.0);

    fc = x*x*sin(10.0*uTime) + y*y*cos(10.0*uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += 0.5*vec4(w[0]-w[1], 0.0, w[0]-w[1], 0.0);

    // dynamic
    fc = x*x*x*sin(uTime) + y*cos(uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += 0.75*vec4(0.0, w[0]-w[1], vec2(0.0));

    fc = x*x*x*sin(2.0*uTime) + y*cos(2.0*uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += 0.5*vec4(0.0, w[0]-w[1], vec2(0.0));

    fc = x*x*x*sin(4.0*uTime) + y*cos(4.0*uTime);
    w[0] = smoothstep(0.04, 0.06, fc);
    w[1] = smoothstep(0.045, 0.06, fc);
    fColor += vec4(0.0, w[0]-w[1], vec2(0.0));
}
