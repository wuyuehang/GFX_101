#include "VulkanCore.hpp"
#include "VulkanSwapchain.hpp"

namespace common {

void VulkanSwapchain::init() {
    const VkInstance & instance = core->get_instance();
    const VkPhysicalDevice & pdev = core->get_pdev();
    const VkDevice & dev = core->get_dev();
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    uint32_t nof_surface_format;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &nof_surface_format, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_fmt(nof_surface_format);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surface, &nof_surface_format, surface_fmt.data());
    for (auto & it : surface_fmt) {
        if (it.format == VK_FORMAT_B8G8R8A8_SRGB && it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            break;
        }
    }
    uint32_t nof_present_mode;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &nof_present_mode, nullptr);
    std::vector<VkPresentModeKHR> present_mode(nof_present_mode);
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &nof_present_mode, present_mode.data());
    for (auto & it : present_mode) {
        if (it == VK_PRESENT_MODE_FIFO_KHR) {
            break;
        }
    }
    VkSurfaceCapabilitiesKHR surface_cap;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &surface_cap);

    VkSwapchainCreateInfoKHR chainInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = surface_cap.minImageCount,
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = surface_cap.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = surface_cap.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_FALSE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    vkCreateSwapchainKHR(dev, &chainInfo, nullptr, &swapchain);

    uint32_t nof_swapchain_image;
    vkGetSwapchainImagesKHR(dev, swapchain, &nof_swapchain_image, nullptr);
    images.resize(nof_swapchain_image);
    vkGetSwapchainImagesKHR(dev, swapchain, &nof_swapchain_image, images.data());

    views.resize(nof_swapchain_image);
    for (uint8_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo imageviewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCreateImageView(dev, &imageviewInfo, nullptr, &views[i]);
    }
}

void VulkanSwapchain::deinit() {
    const VkInstance & instance = core->get_instance();
    const VkDevice & dev = core->get_dev();
    for (auto & it : views) {
        vkDestroyImageView(dev, it, nullptr);
    }
    vkDestroySwapchainKHR(dev, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
}
}