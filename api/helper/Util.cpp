#include "Util.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

namespace gltest {

static void loadGLSL(const std::string filename, std::string & code) {
    std::string line;
    std::ifstream f(filename);
    assert(f.is_open());

    while (std::getline(f, line)) {
        code.append(line);
        code.append(1, '\n');
    }
    f.close();
}

static bool endsWith(const std::string & filename, const std::string & suffix) {
    return std::equal(suffix.rbegin(), suffix.rend(), filename.rbegin());
}

static GLenum parseShaderType(const std::string & filename) {
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

GLuint CreateProgram(std::vector<std::string> & files) {
    std::vector<GLuint> shaders;

    for (auto & file : files) {
        // load source
        std::string code;
        loadGLSL(file, code);
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
    GLuint P = glCreateProgram();
    for (const auto & shader : shaders) {
        glAttachShader(P, shader);
    }

    glLinkProgram(P);

    GLint res;
    glGetProgramiv(P, GL_LINK_STATUS, &res);
    if (!res) {
        GLchar info[512];
        glGetProgramInfoLog(P, 512, NULL, info);
        std::cout << "PROGRAM LINK FAILED\n" << info << std::endl;
        assert(0);
    }

    // clean
    for (auto & shader : shaders) {
        glDeleteShader(shader);
    }

    return P;
}

}
