#version 460

layout(location = 0) in vec4 vPos;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec2 fUV;

#define IMPLICIT 0

#if IMPLICIT
layout(binding = 0) uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
};
#else
layout(set = 0, binding = 0) uniform UBO {
	mat4 model;
	mat4 view;
	mat4 proj;
} MVP;
#endif

void main() {
#if IMPLICIT
	gl_Position = proj * view * model * vPos;
#else
	gl_Position = MVP.proj * MVP.view * MVP.model * vPos;
#endif
	fUV = vUV;
}
