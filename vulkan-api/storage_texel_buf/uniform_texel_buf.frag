#version 460

layout (location = 0) out vec4 SV_Target;
//layout (set = 0, binding = 0) uniform samplerBuffer texelbuf;
layout (set = 0, binding = 0) uniform textureBuffer texelbuf; // the same syntax

void main() {
    int cur_row = int(gl_FragCoord.y - 0.5);
    int cur_col = int(gl_FragCoord.x - 0.5);

#if 0
    // buffer view inteprets the buffer as R8_UNORM
    int addr = cur_row * 800 * 4 + cur_col * 4;
    SV_Target.r = texelFetch(texelbuf, addr).r;
    SV_Target.g = texelFetch(texelbuf, addr + 1).r;
    SV_Target.b = texelFetch(texelbuf, addr + 2).r;
    SV_Target.a = texelFetch(texelbuf, addr + 3).r;
#else
    // buffer view inteprets the buffer as R8G8B8A8_UNORM
    int addr = cur_row * 800 + cur_col;
    SV_Target = texelFetch(texelbuf, addr);
#endif

}
