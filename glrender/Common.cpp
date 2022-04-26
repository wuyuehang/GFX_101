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

GLuint BuildShader(const char *filename, GLenum type) {
    std::string code;
    ReadFile(filename, code);
    const GLchar *tok = code.c_str();

    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &tok, NULL);
    glCompileShader(id);

    GLint res;
    glGetShaderiv(id, GL_COMPILE_STATUS, &res);
    if (!res)
    {
        GLchar info[512];
        glGetShaderInfoLog(id, 512, NULL, info);
        std::cout << std::string(filename) << " COMPILATION_FAILED\n" << info << std::endl;
    }
    return id;
}

GLuint BuildProgram(std::vector<GLuint> & shaders) {
    GLuint id = glCreateProgram();
    for (const auto &it : shaders) {
        glAttachShader(id, it);
    }

    glLinkProgram(id);

    GLint res;
    glGetProgramiv(id, GL_LINK_STATUS, &res);
    if (!res) {
        GLchar info[512];
        glGetProgramInfoLog(id, 512, NULL, info);
        std::cout << "PROGRAM LINK FAILED\n" << info << std::endl;
    }
    return id;
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

void UpdateMVPUBO(GLuint ubo, GLuint prog, MVP & mvp) {
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