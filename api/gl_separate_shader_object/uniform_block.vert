#version 460 core

layout (location = 0) in vec3 vPos;

uniform MVP {
    mat4 model_mat;
    mat4 view_mat;
    mat4 proj_mat;
};

void main() {
    gl_Position = proj_mat * view_mat * model_mat * vec4(vPos, 1.0);
}
