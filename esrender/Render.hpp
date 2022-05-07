#ifndef ES_RENDER_HPP
#define ES_RENDER_HPP

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <GLFW/glfw3.h>
#include <map>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Controller.hpp"
#include "Mesh.hpp"
#include "Program.hpp"

class Render {
public:
    Render(const Render &) = delete;
    Render();
    ~Render();
    void InitGLFW();
    void InitImGui();
    void CreateResource();
    void BakeVAO();
    void BakeDefaultPipeline();
    void run_if_default();
    void BakeVVNPipeline();
    void run_if_vvn();
    void BakeDiffuseSpecularPipeline();
    void run_if_diffuse_specular();
    void BakePhongPipeline();
    void run_if_phong();
    void BakeToonPipeline();
    void run_if_toon();
    void BakeCommand();
    void Gameloop();

public:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
private:
    enum {
        DEFAULT_MODE = 0,
        VISUALIZE_VERTEX_NORMAL_MODE,
        DIFFUSE_SPECULAR_MODE,
        PHONG_MODE,
        TOON_MODE,
    };
    int m_exclusive_mode;
    Controller *m_ctrl;
    AssimpMesh mesh;
    float m_roughness;
    std::map<std::string, GLuint> vaos;
    std::map<std::string, Program *> progs;
};

#endif
