#version 460 core

layout (location = 0) out vec4 SV_Target;

uniform float near;
uniform float far;

void main()
{
    // gl_FragCoord.z is in viewport space
    // inverse it back to NDC space
    float ndc_depth = 2.0 * gl_FragCoord.z - 1.0;

    // inverse it NDC space back to view space, check projection transfrom detail
    float view_depth = 2.0 * far * near / ((far + near) - ndc_depth * (far - near));

    // divide z for visualization effect
    view_depth = view_depth / far;
    SV_Target = vec4(view_depth);
}
