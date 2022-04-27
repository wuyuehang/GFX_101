#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "Common.hpp"
#include <GL/glew.h>
#include <string>
#include <vector>


class Program {
public:
    Program() = delete;
    Program(const Program &) = delete;
    Program(std::vector<std::string> &);
    ~Program() {}
    void use() const { glUseProgram(prog); }
    void updateMVPUBO(GLuint ubo, MVP & mvp);
    void setInt(const std::string & name, int value) const;
    void setFloat(const std::string & name, float value) const;
    void setVec3(const std::string & name, glm::vec3 & v) const;
private:
    void readFile(const std::string &, std::string &);
    bool endsWith(const std::string &, const std::string &);
    GLenum parseShaderType(const std::string &);
    GLuint prog;
};

#endif