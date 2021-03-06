#include "Util.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include "stb_image.h"

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
    } else if (endsWith(filename, ".frag")) {
        return GL_FRAGMENT_SHADER;
    } else if (endsWith(filename, ".comp")) {
        return GL_COMPUTE_SHADER;
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

GLuint CreateXFBProgram(std::string file, std::vector<const GLchar *> xfb_list, GLenum mode) {
    // load source
    std::string code;
    loadGLSL(file, code);
    const GLchar *tok = code.c_str();

    // parse type
    GLenum type = parseShaderType(file);
    assert(type == GL_VERTEX_SHADER);

    // compiling
    GLuint vert = glCreateShader(type);
    glShaderSource(vert, 1, &tok, NULL);
    glCompileShader(vert);
    GLint res;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &res);
    if (!res)
    {
        GLchar info[512];
        glGetShaderInfoLog(vert, 512, NULL, info);
        std::cout << file << " COMPILATION_FAILED\n" << info << std::endl;
        assert(0);
    }

    // linking
    GLuint P = glCreateProgram();
    glAttachShader(P, vert);
    glTransformFeedbackVaryings(P, xfb_list.size(), xfb_list.data(), mode);
    glLinkProgram(P);
    glGetProgramiv(P, GL_LINK_STATUS, &res);
    if (!res) {
        GLchar info[512];
        glGetProgramInfoLog(P, 512, NULL, info);
        std::cout << "PROGRAM LINK FAILED\n" << info << std::endl;
        assert(0);
    }

    // clean
    glDeleteShader(vert);

    return P;
}

#if GL_BACKEND
static const char *drawtexture_vertex = "#version 460 core\n"
    "layout (location = 0) in vec2 vPos;\n"
    "layout (location = 1) in vec2 vUV;\n"
    "layout (location = 0) out vec2 fUV;\n"
    "void main() {\n"
    "   gl_Position = vec4(vPos, 0.0, 1.0);\n"
    "   fUV = vUV;\n"
    "}";

static const char *drawtexture_fragment = "#version 460 core\n"
    "layout (location = 0) in vec2 fUV;\n"
    "layout (location = 0) out vec4 SV_Target;\n"
    "uniform sampler2D TEX0_DIFFUSE;\n"
    "void main() {\n"
    "   SV_Target = texture(TEX0_DIFFUSE, fUV);\n"
    "}";
#endif

#if ES_BACKEND
static const char *drawtexture_vertex = "#version 320 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec2 vPos;\n"
    "layout (location = 1) in vec2 vUV;\n"
    "layout (location = 0) out vec2 fUV;\n"
    "void main() {\n"
    "   gl_Position = vec4(vPos, 0.0, 1.0);\n"
    "   fUV = vUV;\n"
    "}";

static const char *drawtexture_fragment = "#version 320 es\n"
    "precision highp float;\n"
    "layout (location = 0) in vec2 fUV;\n"
    "layout (location = 0) out vec4 SV_Target;\n"
    "uniform sampler2D TEX0_DIFFUSE;\n"
    "void main() {\n"
    "   SV_Target = texture(TEX0_DIFFUSE, fUV);\n"
    "}";
#endif
void DrawTexture(GLuint target) {
    GLfloat vertex_buf[] = {
        -1.0, -1.0, 0.0, 0.0, // bottom-left
        -1.0, 1.0, 0.0, 1.0,  // top-left
        1.0, 1.0, 1.0, 1.0,   // top-right
        1.0, -1.0, 1.0, 0.0,  // bottom-right
    };

    GLushort index_buf[] = { 0, 1, 2, 0, 2, 3 };
    static GLuint _vao = 0;
    static GLuint _vbo, _ibo, _vs, _fs, _p;

    if (_vao == 0) {
        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buf), vertex_buf, GL_STATIC_DRAW);
        glGenBuffers(1, &_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buf), index_buf, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (const void *)(2*sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // compiling
        _vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(_vs, 1, &drawtexture_vertex, NULL);
        glCompileShader(_vs);
        GLint res;
        glGetShaderiv(_vs, GL_COMPILE_STATUS, &res);
        if (!res)
        {
            GLchar info[512];
            glGetShaderInfoLog(_vs, 512, NULL, info);
            std::cout << "COMPILATION_FAILED\n" << info << std::endl;
            assert(0);
        }

        _fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(_fs, 1, &drawtexture_fragment, NULL);
        glCompileShader(_fs);
        glGetShaderiv(_fs, GL_COMPILE_STATUS, &res);
        if (!res)
        {
            GLchar info[512];
            glGetShaderInfoLog(_fs, 512, NULL, info);
            std::cout << "COMPILATION_FAILED\n" << info << std::endl;
            assert(0);
        }
        // linking
        _p = glCreateProgram();
        glAttachShader(_p, _vs);
        glAttachShader(_p, _fs);
        glLinkProgram(_p);
        glGetProgramiv(_p, GL_LINK_STATUS, &res);
        if (!res) {
            GLchar info[512];
            glGetProgramInfoLog(_p, 512, NULL, info);
            std::cout << "PROGRAM LINK FAILED\n" << info << std::endl;
            assert(0);
        }
        glDeleteShader(_vs);
        glDeleteShader(_fs);
    } else {
        glBindVertexArray(_vao);
    }

    glUseProgram(_p);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target);
    glUniform1i(glGetUniformLocation(_p, "TEX0_DIFFUSE"), 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

GLuint CreateCubemap(std::string dir) {
    std::vector<std::string> faces = {
        "right.png", "left.png", "top.png",
        "bottom.png", "back.png", "front.png"
    };
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    int w, h, c;
    uint8_t *ptr;
    for (auto i = 0; i < 6; i++) {
        std::string fullname = dir + faces[i];
        ptr = stbi_load(fullname.c_str(), &w, &h, &c, STBI_rgb_alpha);
        assert(ptr);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
        stbi_image_free(ptr);
        ptr = nullptr;
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return cubemap;
}

}
