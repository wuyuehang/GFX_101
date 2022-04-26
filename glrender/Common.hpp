#ifndef COMMON_HPP
#define COMMON_HPP

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <vector>

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

void ReadFile(const char *filename, std::string & source);
GLuint BuildShader(const char *filename, GLenum type);
GLuint BuildProgram(std::vector<GLuint> & shaders);
GLuint BuildShaderProgram(const char *filename, GLenum type);
GLuint BuildProgramPipeline();

void UpdateMVPUBO(GLuint ubo, GLuint prog, MVP & mvp);

#endif