#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNor;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTan;
layout (location = 4) in vec3 vBta;

layout (location = 0) out vec3 Pos_viewspace;
layout (location = 1) out vec2 UV;
layout (location = 2) out vec3 LightDir_tangentspace;
layout (location = 3) out vec3 ViewDir_tangentspace;

uniform vec3 LightPos_viewspace;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);

    // calculate object position in viewspace
    Pos_viewspace = vec3(view_mat * model_mat * vec4(vPos, 1.0));

    // calculate light direction in viewspace
    vec3 LightDir_viewspace = LightPos_viewspace - Pos_viewspace;

    // passthrough
    UV = vUV;

    // TBN (transform tangent space to view-model space)
    vec3 Tan_viewspace = mat3(view_mat * model_mat) * vTan;
    vec3 Bta_viewspace = mat3(view_mat * model_mat) * vBta;
    vec3 Nor_viewspace = mat3(transpose(inverse(view_mat * model_mat))) * vNor;

    mat3 TBN = mat3(
        Tan_viewspace, // first column
        Bta_viewspace, // second column
        Nor_viewspace // third column
    );

#if 1
    mat3 invTBN = transpose(TBN);
#else
    mat3 invTBN = inverse(TBN);
#endif

    // we do lighting in tangent space
    LightDir_tangentspace = invTBN * LightDir_viewspace;

    // heightmap in tangent space
    ViewDir_tangentspace = invTBN * (vec3(0.0) - Pos_viewspace);
}
