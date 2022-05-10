#ifndef PBRRENDER_HPP
#define PBRRENDER_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Controller.hpp"
#include "GltfMesh.hpp"
#include "Program.hpp"

class PBRRender {
public:
    PBRRender(const PBRRender &) = delete;
    PBRRender(int argc, char *argv[]);
    ~PBRRender();
    void InitGLFW();
    void InitImGui();
    void BakeVAO();
    void BakeDefaultPipeline();
    void run_if_default();
    void BakeWireframePipeline();
    void run_if_wireframe();
    void BakeDiffuseSpecularPipeline();
    void run_if_diffuse_specular();
    void BakePhongPipeline();
    void run_if_phong();
    void BakeCommand();
    void Gameloop();

public:
    GLFWwindow *m_window;
    int32_t m_width;
    int32_t m_height;
private:
    enum {
        DEFAULT_MODE = 0,
        WIREFRAME_MODE,
        DIFFUSE_SPECULAR_MODE,
        PHONG_MODE,
    };
    int m_exclusive_mode;
    Controller *m_ctrl;
    GltfMesh mesh;
    float m_roughness;
    std::map<std::string, GLuint> vaos;
    std::map<std::string, Program *> progs;
};
#endif
