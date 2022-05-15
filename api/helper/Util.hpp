#ifndef UTIL_HPP
#define UTIL_HPP

#include <GL/glew.h>
#include <string>
#include <vector>

namespace gltest {

GLuint CreateProgram(std::vector<std::string> & files);
GLuint CreateXFBProgram(std::string file, std::vector<const GLchar *> xfb_list, GLenum mode);
void DrawTexture(GLuint target);
GLuint CreateCubemap(std::string dir);

}
#endif
