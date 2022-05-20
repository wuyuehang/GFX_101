#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#if GL_BACKEND
#include <GL/glew.h>
#endif

#if ES_BACKEND
#include <GLES3/gl32.h>
#endif

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace util {
class Program {
public:
    Program() = delete;
    Program(const Program &) = delete;
    Program(std::vector<std::string> &);
    ~Program() {}
    void use() const { glUseProgram(prog); }
    GLuint program() const { return prog; }
    void setInt(const std::string & name, int value) const;
    void setFloat(const std::string & name, float value) const;
    void setVec3(const std::string & name, glm::vec3 & v) const;
    void setMat4(const std::string & name, const glm::mat4 & v) const;
    void setFloatArray(const std::string & name, GLsizei count, GLfloat *value) const;
private:
    void readFile(const std::string &, std::string &);
    bool endsWith(const std::string &, const std::string &);
    GLenum parseShaderType(const std::string &);
    GLuint prog;
};
}

#endif
