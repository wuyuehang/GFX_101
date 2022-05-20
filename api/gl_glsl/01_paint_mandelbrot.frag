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
    int i = 0;

    for (i = 0; i < int(_max_mandelbrot_iter); i++) {

        mold = z_real*z_real + z_image*z_image;

        if (mold > _threshold) {
            infinite = true;
            break;
        }

        // f(zn) = z*z + c;
        nz_real = z_real*z_real - z_image*z_image + c_real;
        nz_image = 2.0*z_real*z_image + c_image;

        z_real = nz_real;
        z_image = nz_image;
    }

    if (infinite) {
        fColor += vec4(0.5*sin(i)+0.5, 0.5*cos(i)+0.5, -0.5*sin(i)+0.5, 0.0);
    } else {
        fColor += vec4(mod(z_real, 0.25*sin(1.5*uTime)+0.25), mod(z_image, 0.25*cos(1.5*uTime)+0.25), 0.0, 0.0);
    }
}