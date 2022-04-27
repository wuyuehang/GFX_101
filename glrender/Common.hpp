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
GLuint BuildShaderProgram(const char *filename, GLenum type);
GLuint BuildProgramPipeline();

#endif