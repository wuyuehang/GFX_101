#version 460 core

#define PI 3.141592657589793

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

void main() {
    float w[2];	// weight
    float fx;	// f(x)
    fColor = vec4(0.0);

    // map coordinate from [0, 1] to [-1, 1]
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;
    x = 2.0*x-1.0;
    y = 2.0*y-1.0;

    // paint y=x, red
    fx = x;
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(w[0]-w[1], vec3(0.0));

    // paint y=x^2, green
    fx = x*x;
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(0.0, w[0]-w[1], vec2(0.0));

    // paint y=x^3, blue
    fx = x*x*x;
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(0.0, 0.0, w[0]-w[1], 0.0);

    // paint y=sin(x), yellow
    fx = 0.25*sin(20.0*x);
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(vec2(w[0]-w[1]), vec2(0.0));

    // paint y=tan(x)
    fx = 0.05*tan(10.0*x);
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(0.0, vec2(w[0]-w[1]), 0.0);

    // paint y=log(x+0.25)
    fx = log(x+0.25);
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(w[0]-w[1], 0.0, w[0]-w[1], 0.0);

    // paint y=1/x
    fx = 0.01/x;
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(w[0]-w[1]);

    // paint y=mod(x, 0.1), jigsaw
    fx = mod(x, 0.1);
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(0.0, w[0]-w[1], vec2(0.0));

    // paint
    fx = 0.05*sin(20.0*x)/x;
    w[0] = smoothstep(fx-0.02, fx, y);
    w[1] = smoothstep(fx, fx+0.02, y);
    fColor += vec4(w[0]-w[1], 0.0, w[0]-w[1], 0.0);
}
