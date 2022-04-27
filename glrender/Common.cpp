#include "Common.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

void ReadFile(const char *filename, std::string& source) {
    std::string line;

    std::ifstream f(filename);

    while (std::getline(f, line)) {
        source.append(line);
        source.append(1, '\n');
    }

    f.close();
    return;
}

GLuint BuildShaderProgram(const char *filename, GLenum type) {
    std::string code;
    ReadFile(filename, code);
    const GLchar *tok = code.c_str();

    return glCreateShaderProgramv(type, 1, &tok);
}

GLuint BuildProgramPipeline() {
    GLuint id;
    glGenProgramPipelines(1, &id);
    glBindProgramPipeline(id);
    return id;
}