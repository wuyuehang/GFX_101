#include "Program.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

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
    }else if (endsWith(filename, ".frag")) {
        return GL_FRAGMENT_SHADER;
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

void Program::updateMVPUBO(GLuint ubo, MVP & mvp) {
    GLuint ubo_idx = glGetUniformBlockIndex(prog, "UBO");
    assert(ubo_idx != GL_INVALID_INDEX);

    GLint ubo_size;
    glGetActiveUniformBlockiv(prog, ubo_idx, GL_UNIFORM_BLOCK_DATA_SIZE, &ubo_size);
    assert(ubo_size == sizeof(MVP));

    std::vector<const char *> names { "UBO.model", "UBO.view", "UBO.proj" }; // uniform block name, not the instance name
    std::vector<GLuint> indices(names.size());
    std::vector<GLint> array_sizes(names.size());
    std::vector<GLint> offsets(names.size());
    std::vector<GLint> types(names.size());

    glGetUniformIndices(prog, names.size(), names.data(), indices.data());
    glGetActiveUniformsiv(prog, names.size(), indices.data(), GL_UNIFORM_OFFSET, offsets.data());
    glGetActiveUniformsiv(prog, names.size(), indices.data(), GL_UNIFORM_SIZE, array_sizes.data());
    glGetActiveUniformsiv(prog, names.size(), indices.data(), GL_UNIFORM_TYPE, types.data());

#if 0
    for (int i = 0; i < 3; i++) {
        std::cout << names[i] << std::endl;
        std::cout << "indice : " << indices[i] << std::endl;
        std::cout << "offset : " << offsets[i] << std::endl;
        std::cout << "array  : " << array_sizes[i] << std::endl;
        std::cout << "type   : " << types[i] << std::endl;
    }
#endif

    GLubyte *buffer = (GLubyte *)malloc(ubo_size);
    std::memcpy(buffer + offsets[0], &mvp.model[0][0], array_sizes[0] * sizeof(glm::mat4));
    std::memcpy(buffer + offsets[1], &mvp.view[0][0], array_sizes[1] * sizeof(glm::mat4));
    std::memcpy(buffer + offsets[2], &mvp.proj[0][0], array_sizes[2] * sizeof(glm::mat4));

    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, ubo_size, buffer, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, ubo_idx, ubo);

    free(buffer);
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