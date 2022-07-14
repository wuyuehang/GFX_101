#if ES_BACKEND
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#endif

#if GL_BACKEND
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

int main() {
#if GL_BACKEND
    // check OPENGL extension
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        GLFWwindow *window = glfwCreateWindow(64, 64, "ext logger", nullptr, nullptr);
        glfwMakeContextCurrent(window);
        glewInit();
        int nbr_of_gl_ext;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nbr_of_gl_ext);
        std::cout << "OpenGL extensions..." << std::endl;
        for (auto i = 0; i < nbr_of_gl_ext; i++) {
            auto ext = glGetStringi(GL_EXTENSIONS, i);
            std::cout << "\t" << ext << std::endl;
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }
#endif

#if ES_BACKEND
    // check GLES extension
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        GLFWwindow *window = glfwCreateWindow(64, 64, "ext logger", nullptr, nullptr);
        glfwMakeContextCurrent(window);

        int nbr_of_es_ext;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nbr_of_es_ext);
        std::cout << "OpenGL ES extensions..." << std::endl;
        for (auto i = 0; i < nbr_of_es_ext; i++) {
            auto ext = glGetStringi(GL_EXTENSIONS, i);
            std::cout << "\t" << ext << std::endl;
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }
#endif
    // check VULKAN extension
    {
        VkInstance instance;
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pApplicationInfo = &app_info;
        vkCreateInstance(&instance_info, nullptr, &instance);

        uint32_t pdev_count;
        vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
        std::vector<VkPhysicalDevice> physical_dev(pdev_count);
        vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
        VkPhysicalDevice pdev = physical_dev[0];

        uint32_t nbr_of_vk_ext;
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &nbr_of_vk_ext, nullptr);
        std::vector<VkExtensionProperties> dev_ext(nbr_of_vk_ext);
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &nbr_of_vk_ext, dev_ext.data());
        std::cout << "Vulkan device extensions..." << std::endl;
        for (auto & it : dev_ext) {
            std::cout << "\t" << it.extensionName << std::endl;
        }
        vkDestroyInstance(instance, nullptr);
    }

    return 0;
}
