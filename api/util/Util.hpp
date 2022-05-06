#ifndef UTIL_HPP
#define UTIL_HPP

#include <GL/glew.h>
#include <string>
#include <vector>

namespace util {

GLuint CreateProgram(std::vector<std::string> & files);

}
#endif