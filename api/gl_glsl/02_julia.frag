#version 460 core

uniform uvec2 uResInfo;
uniform float uTime;
uniform vec2 uMouse;

out vec4 fColor;

#define _min_x -1.5
#define _max_x 1.5
#define _min_y -1.5
#define _max_y 1.5

#define _max_mandelbrot_iter 20.0

#define downA 0.5
#define upA 2.0

void main() {
    // animate _threshold
    float _threshold = 0.5*(upA-downA)*sin(1.25*uTime)+0.5*(downA+upA);

    // normalize to internal [0, 1]
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;

    // remap the internal
    float z_real = (_max_x - _min_x)*x + _min_x;
    float z_image = (_max_y - _min_y)*y + _min_y;
    float nz_real;
    float nz_image;

    #if 1
        float c_real = uMouse.x;
        float c_image = uMouse.y;
    #else
        float c_real = 0.45;
        float c_image = -0.1;
    #endif

    fColor = vec4(0.15, 0.05, 0.15, 0.0);

    bool infinite = false;
    float mold;

    int i;

    for (i = 0; i < int(_max_mandelbrot_iter); i++) {

        mold = z_real*z_real + z_image*z_image;

        if (mold > _threshold) {
            infinite = true;
            break;
        }

        // f(zn) = z*z + c
        nz_real = z_real*z_real - z_image*z_image + c_real;
        nz_image = 2.0*z_real*z_image + c_image;

        z_real = nz_real;
        z_image = nz_image;
    }

    if (infinite) {
        fColor += vec4(i/_max_mandelbrot_iter, 0.0, 0.0, 0.0);
    }
}
