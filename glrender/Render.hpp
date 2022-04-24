#ifndef RENDER_HPP
#define RENDER_HPP

#include <GLFW/glfw3.h>

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void Gameloop();

private:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
};
#endif