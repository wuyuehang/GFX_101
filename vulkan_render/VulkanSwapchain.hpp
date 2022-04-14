#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cassert>
#include <vector>

class HelloVulkan;
class VulkanSwapchain {
public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(const VulkanSwapchain &) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain &) = delete;
    VulkanSwapchain(HelloVulkan* app) : m_hello_vulkan(app) { assert(app != nullptr); }
    void init();
    void deinit();
public:
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
private:
    HelloVulkan* m_hello_vulkan;
    VkSurfaceKHR surface;
};
#endif