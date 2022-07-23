#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cassert>
#include <vector>

namespace common {

class VulkanCore;
class VulkanSwapchain {
public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(const VulkanSwapchain &) = delete;
    VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
    VulkanSwapchain(const VulkanCore *app, GLFWwindow *win) : core(app), window(win) { assert(app != nullptr && win != nullptr); }
    void init();
    void deinit();
public:
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
private:
    const VulkanCore *core;
    VkSurfaceKHR surface;
    GLFWwindow *window;
};
}
#endif