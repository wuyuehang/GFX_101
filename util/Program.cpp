#include "Program.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

namespace util {
void Program::readFile(const std::string & filename, std::string & source) {
    std::string line;
    std::ifstream f(filename.c_str());

    while (std::getline(f, line)) {
        source.append(line);
        source.append(1, '\n');
    }

    f.close();
    return;
}

bool Program::endsWith(const std::string & filename, const std::string & suffix) {
    return std::equal(suffix.rbegin(), suffix.rend(), filename.rbegin());
}

GLenum Program::parseShaderType(const std::string & filename) {
    if (endsWith(filename, ".vert")) {
        return GL_VERTEX_SHADER;
    } else if (endsWith(filename, ".geom")) {
        return GL_GEOMETRY_SHADER;
    } else if (endsWith(filename, ".frag")) {
        return GL_FRAGMENT_SHADER;
    } else if (endsWith(filename, ".tesc")) {
        return GL_TESS_CONTROL_SHADER;
    } else if (endsWith(filename, ".tese")) {
        return GL_TESS_EVALUATION_SHADER;
    } else {
        assert(0);
    }
}

Program::Program(std::vector<std::string> & files) {
    std::vector<GLuint> shaders;

    for (auto & file : files) {
        // load source
        std::string code;
        readFile(file, code);
        const GLchar *tok = code.c_str();

        // parse type
        GLenum type = parseShaderType(file);

        // compiling
        GLuint id = glCreateShader(type);
        glShaderSource(id, 1, &tok, NULL);
        glCompileShader(id);
        GLint res;
        glGetShaderiv(id, GL_COMPILE_STATUS, &res);
        if (!res)
        {
            GLchar info[512];
            glGetShaderInfoLog(id, 512, NULL, info);
            std::cout << file << " COMPILATION_FAILED\n" << info << std::endl;
            assert(0);
        }
        shaders.push_back(id);
    }

    // linking
    prog = glCreateProgram();
    for (const auto & shader : shaders) {
        glAttachShader(prog, shader);
    }

    glLinkProgram(prog);

    GLint res;
    glGetProgramiv(prog, GL_LINK_STATUS, &res);
    if (!res) {
        GLchar info[512];
        glGetProgramInfoLog(prog, 512, NULL, info);
        std::cout << "PROGRAM LINK FAILED\n" << info << std::endl;
        assert(0);
    }

    // clean
    for (auto & shader : shaders) {
        glDeleteShader(shader);
    }
}

void Program::setInt(const std::string & name, int value) const {
    glProgramUniform1i(prog, glGetUniformLocation(prog, name.c_str()), value);
}

void Program::setFloat(const std::string & name, float value) const {
    glProgramUniform1f(prog, glGetUniformLocation(prog, name.c_str()), value);
}

void Program::setVec3(const std::string & name, glm::vec3 & value) const {
    glProgramUniform3fv(prog, glGetUniformLocation(prog, name.c_str()), 1, &value[0]);
}

void Program::setMat4(const std::string & name, const glm::mat4 & value) const {
    glProgramUniformMatrix4fv(prog, glGetUniformLocation(prog, name.c_str()), 1, GL_FALSE, &value[0][0]);
}
}
