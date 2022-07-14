#ifndef UTIL_HPP
#define UTIL_HPP

#if GL_BACKEND
#include <GL/glew.h>
#endif

#if ES_BACKEND
#include <GLES3/gl32.h>
#endif

#include <string>
#include <vector>

namespace gltest {

GLuint CreateProgram(std::vector<std::string> & files);
GLuint CreateXFBProgram(std::string file, std::vector<const GLchar *> xfb_list, GLenum mode);
void DrawTexture(GLuint target);
GLuint CreateCubemap(std::string dir);

}
#endif
