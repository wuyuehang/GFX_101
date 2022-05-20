#version 460 core

uniform uvec2 uResInfo;
uniform float uTime;

out vec4 fColor;

#define _left_internal -1.4
#define _right_internal 1.4

#define downA 2.0
#define upA 2.0

#define max_iter_left 2.0
#define max_iter_right 50.0

void main() {
    // animate _max_mandelbrot_iter
    float _max_mandelbrot_iter = 0.5*(max_iter_right-max_iter_left)*sin(1.5*uTime)+0.5*(max_iter_left+max_iter_right);

    // animate _threshold
    float _threshold = 0.5*(upA-downA)*sin(1.25*uTime)+0.5*(downA+upA);

    // normalize
    float x = gl_FragCoord.x / uResInfo.x;
    float y = gl_FragCoord.y / uResInfo.y;

    // curret remap (x, y) represent candidate complex c
    // remap 2d domain to [_right_internal, _left_internal]
    float c_real = (_right_internal - _left_internal)*x + _left_internal;
    float c_image = (_right_internal - _left_internal)*y + _left_internal;

    fColor = vec4(0.02, 0.01, 0.02, 0.0);
    bool infinite = false;

    float z_real = c_real;
    float z_image = c_image;
    float nz_real = 0.0;
    float nz_image = 0.0;
    float mold;
    int stopat = 0;

    for (int i = 0; i < int(_max_mandelbrot_iter); i++) {

        mold = z_real*z_real + z_image*z_image;

        if (mold > _threshold) {
            infinite = true;
            stopat = i;
            break;
        }

        // f(zn) = z*z + c;
        nz_real = z_real*z_real - z_image*z_image + c_real;
        nz_image = 2.0*z_real*z_image + c_image;

        z_real = nz_real;
        z_image = nz_image;
    }

    if (infinite) {
        fColor = +0.75*vec4(1.0-float(stopat)/_max_mandelbrot_iter, 0.0, 0.0, 0.0);
    }
}