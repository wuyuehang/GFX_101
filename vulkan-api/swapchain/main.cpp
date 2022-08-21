#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "VulkanSwapchain.hpp"
#include "SkeletonVulkan.hpp"

namespace common {

class TestSwapchain : public SkeletonVulkan {
public:
    TestSwapchain() : SkeletonVulkan(VK_API_VERSION_1_0) {
        SkeletonVulkan::init();
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        GLFWwindow *window = glfwCreateWindow(100, 100, "TestSwapchain", nullptr, nullptr);
        VulkanSwapchain *swapchain = new VulkanSwapchain(this, window);
        swapchain->init();
        swapchain->deinit();
        delete swapchain;
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    ~TestSwapchain() {}
};

}
int main() {
    common::TestSwapchain T;
    return 0;
}
