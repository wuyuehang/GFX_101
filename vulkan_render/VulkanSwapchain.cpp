#include "HelloVulkan.hpp"
#include "VulkanCommon.hpp"

void VulkanSwapchain::init() {
    const VkInstance &instance = m_hello_vulkan->instance;
    const VkPhysicalDevice &pdev = m_hello_vulkan->pdev;
    const VkDevice &dev = m_hello_vulkan->dev;
    GLFWwindow *window = m_hello_vulkan->window;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    uint32_t surface_fmt_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &surface_fmt_count, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_fmt(surface_fmt_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &surface_fmt_count, surface_fmt.data());
    for (auto& it : surface_fmt) {
        if (it.format == VK_FORMAT_B8G8R8A8_SRGB && it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            break;
        }
    }
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_mode(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &present_mode_count, present_mode.data());
    for (auto& it : present_mode) {
        if (it == VK_PRESENT_MODE_FIFO_KHR) {
            break;
        }
    }
    VkSurfaceCapabilitiesKHR surface_cap;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &surface_cap);

    VkSwapchainCreateInfoKHR chain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    chain_info.surface = surface;
    chain_info.minImageCount = surface_cap.minImageCount;
    chain_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    chain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    chain_info.imageExtent = surface_cap.currentExtent;
    chain_info.imageArrayLayers = 1;
    chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    chain_info.queueFamilyIndexCount = 0;
    chain_info.pQueueFamilyIndices = nullptr;
    chain_info.preTransform = surface_cap.currentTransform;
    chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    chain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    chain_info.oldSwapchain = VK_NULL_HANDLE;
    vkCreateSwapchainKHR(dev, &chain_info, nullptr, &swapchain);

    uint32_t swapchain_image_count;
    vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, nullptr);
    images.resize(swapchain_image_count);
    vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, images.data());

    views.resize(swapchain_image_count);
    for (uint8_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo imageview_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageview_info.image = images[i];
        imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageview_info.format = VK_FORMAT_B8G8R8A8_SRGB;
        imageview_info.components.r = VK_COMPONENT_SWIZZLE_R;
        imageview_info.components.g = VK_COMPONENT_SWIZZLE_G;
        imageview_info.components.b = VK_COMPONENT_SWIZZLE_B;
        imageview_info.components.a = VK_COMPONENT_SWIZZLE_A;
        imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageview_info.subresourceRange.baseMipLevel = 0;
        imageview_info.subresourceRange.levelCount = 1;
        imageview_info.subresourceRange.baseArrayLayer = 0;
        imageview_info.subresourceRange.layerCount = 1;
        vkCreateImageView(dev, &imageview_info, nullptr, &views[i]);
    }
}

void VulkanSwapchain::deinit() {
    const VkInstance &instance = m_hello_vulkan->instance;
    const VkDevice &dev = m_hello_vulkan->dev;
    for (auto& it : views) {
        vkDestroyImageView(dev, it, nullptr);
    }
    vkDestroySwapchainKHR(dev, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
}