#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define USE_COMBINED_IMAGE_SAMPLER 0

class HelloVulkan {
public:
    HelloVulkan() {
        InitGLFW();
        CreateInstance();
        glfwCreateWindowSurface(instance, window, nullptr, &surface); // create a presentation surface
        CreateDevice();
        CreateSwapchain();
        InitSync();
        CreateCommand();
        CreateRenderPass();
        CreateFramebuffer();
        CreateVertexBuffer();
        BakeTexture();
        CreatePipeline();
        BakeCommand();
    }
    ~HelloVulkan() {
        vkDeviceWaitIdle(dev);
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, layout, nullptr);
        vkDestroySampler(dev, sampler, nullptr);
        vkDestroyImageView(dev, tex_imageview, nullptr);
        vkFreeMemory(dev, tex_mem, nullptr);
        vkDestroyImage(dev, tex_image, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, pool, 1, &ds);
        vkDestroyDescriptorPool(dev, pool, nullptr);
        vkFreeMemory(dev, interleaved_bufmem, nullptr);
        vkDestroyBuffer(dev, interleaved_buf, nullptr);

        for (auto& it : framebuffers) {
            vkDestroyFramebuffer(dev, it, nullptr);
        }
        vkDestroyRenderPass(dev, rp, nullptr);
        vkFreeCommandBuffers(dev, cmdpool, cmdbuf.size(), cmdbuf.data());
        vkDestroyCommandPool(dev, cmdpool, nullptr);
        vkDestroySemaphore(dev, image_available_sema, nullptr);
        vkDestroySemaphore(dev, image_render_finished_sema, nullptr);
        for (auto& it : swapchain_imageviews) {
            vkDestroyImageView(dev, it, nullptr);
        }
        vkDestroySwapchainKHR(dev, swapchain, nullptr);
        vkDestroyDevice(dev, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    std::vector<char> loadSPIRV(const std::string str) {
        std::ifstream f(str, std::ios::ate | std::ios::binary);
        size_t size = (size_t)f.tellg();
        std::vector<char> buf(size);
        f.seekg(0);
        f.read(buf.data(), size);
        f.close();
        return buf;
    }
    void InitGLFW() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(800, 800, "texture", nullptr, nullptr);
    }
    void CreateInstance() {
        // dynamically fetch vulkan function by dlopen libvulkan.
        // no need to include vulkan header, otherwise it will alias
        // #define vk_global_func( fun ) fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun)
        // vk_global_func( vkCreateInstance );
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pApplicationInfo = &app_info;
        std::vector<const char *> instance_extension;
        instance_extension.push_back("VK_KHR_surface"); // instance level extension to enable render onscreen images.
        instance_extension.push_back("VK_KHR_xcb_surface");
        instance_info.ppEnabledExtensionNames = instance_extension.data();
        instance_info.enabledExtensionCount = instance_extension.size();
        vkCreateInstance(&instance_info, nullptr, &instance);
    }
    void CreateDevice() {
        uint32_t pdev_count;
        vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
        std::vector<VkPhysicalDevice> physical_dev(pdev_count);
        vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
        pdev = physical_dev[0];

        uint32_t queue_prop_count;
        q_family_index = 0; // reference
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_props(queue_prop_count);
        vkGetPhysicalDeviceQueueFamilyProperties(pdev, &queue_prop_count, queue_props.data());
        for (auto& it : queue_props) {
            if (it.queueFlags & VK_QUEUE_GRAPHICS_BIT && it.queueFlags & VK_QUEUE_COMPUTE_BIT && it.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                break;
            }
            q_family_index++;
        }

        VkDeviceQueueCreateInfo queue_info { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        float q_prio = 1.0;
        queue_info.queueFamilyIndex = q_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &q_prio;

        VkDeviceCreateInfo dev_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        std::vector<const char *> dev_ext;
        dev_ext.push_back("VK_KHR_swapchain"); // device level extension to enable render onscreen images.
        dev_info.queueCreateInfoCount = 1;
        dev_info.pQueueCreateInfos = &queue_info;
        dev_info.enabledExtensionCount = dev_ext.size();
        dev_info.ppEnabledExtensionNames = dev_ext.data();

        vkCreateDevice(pdev, &dev_info, nullptr, &dev);
        //VkBool32 present_supported = VK_FALSE;
        //vkGetPhysicalDeviceSurfaceSupportKHR(pdev, q_family_index, surface, &present_supported);
        //assert(VK_TRUE == present_supported); // assume the render queue is the same as present queue
        vkGetDeviceQueue(dev, q_family_index, 0, &queue); // we only create 1 queue from the family queues, its index is 0

        vkGetPhysicalDeviceMemoryProperties(pdev, &mem_properties);
    }
    void CreateSwapchain() {
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
        chain_info.surface = surface; // onto which the swapchain will present images (swapchain --> surface)
        chain_info.minImageCount = surface_cap.minImageCount;
        chain_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        chain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        chain_info.imageExtent = surface_cap.currentExtent;
        chain_info.imageArrayLayers = 1;
        chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        chain_info.queueFamilyIndexCount = 0; // exclusive mode
        chain_info.pQueueFamilyIndices = nullptr; // exclusive mode
        chain_info.preTransform = surface_cap.currentTransform; // VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        chain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // similar to double buffers
        chain_info.oldSwapchain = VK_NULL_HANDLE;
        vkCreateSwapchainKHR(dev, &chain_info, nullptr, &swapchain);

        uint32_t swapchain_image_count;
        vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, nullptr);
        swapchain_images.resize(swapchain_image_count);
        vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, swapchain_images.data());

        swapchain_imageviews.resize(swapchain_image_count);
        for (uint8_t i = 0; i < swapchain_images.size(); i++) {
            VkImageViewCreateInfo imageview_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            imageview_info.image = swapchain_images[i];
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
            vkCreateImageView(dev, &imageview_info, nullptr, &swapchain_imageviews[i]);
        }
    }
    void InitSync() {
        VkSemaphoreCreateInfo sema_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_available_sema);
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_render_finished_sema);
    }
    void CreateCommand() {
        VkCommandPoolCreateInfo commandpool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow individual commandbuffer reset
        commandpool_info.queueFamilyIndex = q_family_index;
        vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

        VkCommandBufferAllocateInfo cmdbuf_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cmdbuf_info.commandPool = cmdpool;
        cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdbuf_info.commandBufferCount = swapchain_images.size(); // must be per image
        cmdbuf.resize(swapchain_images.size());
        vkAllocateCommandBuffers(dev, &cmdbuf_info, cmdbuf.data());
    }
    void CreateRenderPass() {
        VkAttachmentDescription att_desc {};
        att_desc.format = VK_FORMAT_B8G8R8A8_SRGB; // same as swapchain
        att_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        att_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image layout changes within a render pass witout image memory barrier
        att_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference att_ref {};
        att_ref.attachment = 0;
        att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription sp_desc {};
        sp_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp_desc.colorAttachmentCount = 1;
        sp_desc.pColorAttachments = &att_ref;

        VkRenderPassCreateInfo rp_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rp_info.attachmentCount = 1;
        rp_info.pAttachments = &att_desc;
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &sp_desc;

        vkCreateRenderPass(dev, &rp_info, nullptr, &rp);
    }
    void CreateFramebuffer() {
        // render pass doesn't specify what images to use
        framebuffers.resize(swapchain_images.size());
        for (uint32_t i = 0; i < swapchain_images.size(); i++) {
            VkFramebufferCreateInfo fb_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            fb_info.renderPass = rp;
            fb_info.attachmentCount = 1;
            fb_info.pAttachments = &swapchain_imageviews[i]; // translate imageview(s) to attachment(s)
            fb_info.width = 800;
            fb_info.height = 800;
            fb_info.layers = 1;
            vkCreateFramebuffer(dev, &fb_info, nullptr, &framebuffers[i]);
        }
    }
    uint32_t findMemoryType(uint32_t memoryTypeBit, VkMemoryPropertyFlags request_prop) {
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if (memoryTypeBit & (1 << i)) { // match the desired memory type (a reported memoryTypeBit could match multiples)
                if ((mem_properties.memoryTypes[i].propertyFlags & request_prop) == request_prop) { // match the request mempry properties
                    return i;
                }
            }
        }
        assert(0);
        return -1;
    }
    void CreateVertexBuffer() {
        const std::vector<float> interleaved_vb_data {
            -1.0, -1.0, 0.0, 1.0, /* Position */ 0.0, 0.0, /* Coordinate */
            1.0, -1.0, 0.0, 1.0, /* Position */ 1.0, 0.0, /* Coordinate */
            -1.0, 1.0, 0.0, 1.0, /* Position */ 0.0, 1.0, /* Cooridnate */
        };
        VkBuffer interleaved_buf_staging;
        VkDeviceMemory interleaved_bufmem_staging;
        VkBufferCreateInfo buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buf_info.size = interleaved_vb_data.size() * sizeof(float);
        buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(dev, &buf_info, nullptr, &interleaved_buf_staging); // CPU access friendly

        VkMemoryRequirements req {};
        vkGetBufferMemoryRequirements(dev, interleaved_buf_staging, &req);

        VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        vkAllocateMemory(dev, &alloc_info, nullptr, &interleaved_bufmem_staging);

        vkBindBufferMemory(dev, interleaved_buf_staging, interleaved_bufmem_staging, 0);

        void *buf_ptr;
        vkMapMemory(dev, interleaved_bufmem_staging, 0, buf_info.size, 0, &buf_ptr);
        memcpy(buf_ptr, interleaved_vb_data.data(), buf_info.size);
        vkUnmapMemory(dev, interleaved_bufmem_staging);
        {
            VkBufferCreateInfo buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            buf_info.size = interleaved_vb_data.size() * sizeof(float);
            buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &buf_info, nullptr, &interleaved_buf); // GPU access friendly

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, interleaved_buf, &req);

            VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &interleaved_bufmem);

            vkBindBufferMemory(dev, interleaved_buf, interleaved_bufmem, 0);
        }

        VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(cmdbuf[0], &cmdbuf_begin_info);
        VkBufferCopy region { 0, 0, buf_info.size };
        vkCmdCopyBuffer(cmdbuf[0], interleaved_buf_staging, interleaved_buf, 1, &region);
        vkEndCommandBuffer(cmdbuf[0]);

        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdbuf[0];

        VkFence fence;
        VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(dev, &fence_info, nullptr, &fence);
        vkQueueSubmit(queue, 1, &submit_info, fence);
        vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(dev, fence, nullptr);
        vkFreeMemory(dev, interleaved_bufmem_staging, nullptr);
        vkDestroyBuffer(dev, interleaved_buf_staging, nullptr);
        vkResetCommandBuffer(cmdbuf[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }
    void BakeTexture() {
        {
            // first, upload the linear content to a staging buffer
            VkBuffer linear_buf;
            VkDeviceMemory linear_buf_mem;
            int w, h, c;
            uint8_t *ptr = stbi_load("../resource/ReneDescartes.jpeg", &w, &h, &c, STBI_rgb_alpha);
            assert(ptr);
            VkBufferCreateInfo linear_buf_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            linear_buf_info.size = sizeof(uint8_t)*w*h*4;
            linear_buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vkCreateBuffer(dev, &linear_buf_info, nullptr, &linear_buf);

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, linear_buf, &req);

            VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &linear_buf_mem);

            vkBindBufferMemory(dev, linear_buf, linear_buf_mem, 0);

            void *buf_ptr;
            vkMapMemory(dev, linear_buf_mem, 0, linear_buf_info.size, 0, &buf_ptr);
            memcpy(buf_ptr, ptr, linear_buf_info.size);
            vkUnmapMemory(dev, linear_buf_mem);
            stbi_image_free(ptr);

            // second, initialize image with optimal layout
            VkImageCreateInfo image_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            image_info.imageType = VK_IMAGE_TYPE_2D;
            image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            image_info.extent = VkExtent3D {static_cast<uint>(w), static_cast<uint>(h), 1};
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
            vkCreateImage(dev, &image_info, nullptr, &tex_image);

            VkMemoryRequirements img_req {};
            vkGetImageMemoryRequirements(dev, tex_image, &img_req);
            VkMemoryAllocateInfo img_alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            img_alloc_info.allocationSize = img_req.size;
            img_alloc_info.memoryTypeIndex = findMemoryType(img_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &img_alloc_info, nullptr, &tex_mem);

            vkBindImageMemory(dev, tex_image, tex_mem, 0);
            VkImageViewCreateInfo imageview_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            imageview_info.image = tex_image;
            imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageview_info.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageview_info.components.r = VK_COMPONENT_SWIZZLE_R;
            imageview_info.components.g = VK_COMPONENT_SWIZZLE_G;
            imageview_info.components.b = VK_COMPONENT_SWIZZLE_B;
            imageview_info.components.a = VK_COMPONENT_SWIZZLE_A;
            imageview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageview_info.subresourceRange.baseMipLevel = 0;
            imageview_info.subresourceRange.levelCount = 1;
            imageview_info.subresourceRange.baseArrayLayer = 0;
            imageview_info.subresourceRange.layerCount = 1;
            vkCreateImageView(dev, &imageview_info, nullptr, &tex_imageview);

            // third, blit buffer into image
            VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(cmdbuf[0], &cmdbuf_begin_info);

            VkImageMemoryBarrier imb { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            imb.srcAccessMask = 0;
            imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imb.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imb.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.image = tex_image;
            imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imb.subresourceRange.baseMipLevel = 0;
            imb.subresourceRange.levelCount = 1;
            imb.subresourceRange.baseArrayLayer = 0;
            imb.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmdbuf[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imb);

            VkBufferImageCopy region {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = VkImageSubresourceLayers {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .imageOffset = VkOffset3D { 0, 0, 0 },
                .imageExtent = VkExtent3D { 800, 800, 1 },
            };
            vkCmdCopyBufferToImage(cmdbuf[0], linear_buf, tex_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region); // must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL

            imb = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imb.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imb.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.image = tex_image;
            imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imb.subresourceRange.baseMipLevel = 0;
            imb.subresourceRange.levelCount = 1;
            imb.subresourceRange.baseArrayLayer = 0;
            imb.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(cmdbuf[0], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imb);
            vkEndCommandBuffer(cmdbuf[0]);

            VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuf[0];

            VkFence fence;
            VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            vkCreateFence(dev, &fence_info, nullptr, &fence);
            vkQueueSubmit(queue, 1, &submit_info, fence);
            vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

            vkDestroyFence(dev, fence, nullptr);
            vkFreeMemory(dev, linear_buf_mem, nullptr);
            vkDestroyBuffer(dev, linear_buf, nullptr);
            vkResetCommandBuffer(cmdbuf[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        }

        VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        vkCreateSampler(dev, &sampler_info, nullptr, &sampler);

#if USE_COMBINED_IMAGE_SAMPLER
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // GLSL f.inst "uniform sampler2D tex;"
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
#else
        std::vector<VkDescriptorSetLayoutBinding> bindings(2);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // GLSL f.inst "uniform texture2D tex;"
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; // GLSL f.inst "uniform sampler smp;"
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
#endif

        VkDescriptorSetLayoutCreateInfo dsl_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dsl_info.bindingCount = bindings.size();
        dsl_info.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);

#if USE_COMBINED_IMAGE_SAMPLER
        std::vector<VkDescriptorPoolSize> pool_size(1);
        pool_size[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size[0].descriptorCount = 1;
#else
        std::vector<VkDescriptorPoolSize> pool_size(2);
        pool_size[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pool_size[0].descriptorCount = 1;
        pool_size[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        pool_size[1].descriptorCount = 1;
#endif

        VkDescriptorPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = pool_size.size();
        pool_info.pPoolSizes = pool_size.data();
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool);

        VkDescriptorSetAllocateInfo alloc_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &dsl;
        vkAllocateDescriptorSets(dev, &alloc_info, &ds);

#if USE_COMBINED_IMAGE_SAMPLER
        VkDescriptorImageInfo img_info {
            .sampler = sampler,
            .imageView = tex_imageview,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        std::vector<VkWriteDescriptorSet> wds(1);
        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = ds;
        wds[0].dstBinding = 0;
        wds[0].dstArrayElement = 0;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds[0].pImageInfo = &img_info;
#else
        VkDescriptorImageInfo img_info {
            .sampler = VK_NULL_HANDLE,
            .imageView = tex_imageview,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorImageInfo smp_info {
            .sampler = sampler,
            .imageView = VK_NULL_HANDLE,
            .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        std::vector<VkWriteDescriptorSet> wds(2);
        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = ds;
        wds[0].dstBinding = 0;
        wds[0].dstArrayElement = 0;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        wds[0].pImageInfo = &img_info;
        wds[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[1].dstSet = ds;
        wds[1].dstBinding = 1;
        wds[1].dstArrayElement = 0;
        wds[1].descriptorCount = 1;
        wds[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        wds[1].pImageInfo = &smp_info;
#endif

        vkUpdateDescriptorSets(dev, wds.size(), wds.data(), 0, nullptr);
    }
    void CreatePipeline() {
        // shaders related blobs
        auto vert_spv = loadSPIRV("simple.vert.spv");
#if USE_COMBINED_IMAGE_SAMPLER
        auto frag_spv = loadSPIRV("combined_image_sampler.frag.spv");
#else
        auto frag_spv = loadSPIRV("separate_image_sampler.frag.spv");
#endif
        VkShaderModule vert, frag;
        VkShaderModuleCreateInfo shader_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shader_info.codeSize = vert_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(vert_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &vert);
        shader_info.codeSize = frag_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(frag_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &frag);

        VkPipelineShaderStageCreateInfo vert_stage_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_stage_info.module = vert;
        vert_stage_info.pName = "main";
        VkPipelineShaderStageCreateInfo frag_stage_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_stage_info.module = frag;
        frag_stage_info.pName = "main";
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_info;
        shader_stage_info.push_back(vert_stage_info);
        shader_stage_info.push_back(frag_stage_info);

        // vertex input
        std::vector<VkVertexInputBindingDescription> vbd(1);
        vbd[0].binding = 0;
        vbd[0].stride = 6*sizeof(float);
        vbd[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> ad(2);
        ad[0].location = 0; // Position
        ad[0].binding = 0; // single vertex attribute buffer
        ad[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        ad[0].offset = 0;
        ad[1].location = 1; // Color
        ad[1].binding = 0;
        ad[1].format = VK_FORMAT_R32G32_SFLOAT;
        ad[1].offset = 4*sizeof(float);

        VkPipelineVertexInputStateCreateInfo vert_input_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vbd.size()),
            .pVertexBindingDescriptions = vbd.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(ad.size()),
            .pVertexAttributeDescriptions = ad.data(),
        };

        // input assembly
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE, // xxx_list must be false.
        };

        // viewport
        VkViewport vp {
            .x = 0,
            .y = 0,
            .width = 800,
            .height = 800,
            .minDepth = 0.0,
            .maxDepth = 1.0,
        };
        VkRect2D scissor {};
        scissor.offset = VkOffset2D { 0, 0 };
        scissor.extent = VkExtent2D { 800, 800 };
        VkPipelineViewportStateCreateInfo vp_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &vp,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        // rasterizer
        VkPipelineRasterizationStateCreateInfo rs_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE, // if the depthClamp device feature is disabled, depthClampEnable must be VK_FALS
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE, // inefficient
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0,
            .depthBiasClamp = 0.0,
            .depthBiasSlopeFactor = 0.0,
            .lineWidth = 1.0,
        };

        // multisample
        VkPipelineMultisampleStateCreateInfo ms_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0.0,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
        };

        // color blend
        // if renderPass is not VK_NULL_HANDLE, the pipeline is being created with fragment output interface state,
        // and subpass uses color attachments, pColorBlendState must be a valid pointer to a valid
        // VkPipelineColorBlendStateCreateInfo structure.
        VkPipelineColorBlendAttachmentState blend_att_state {};
        blend_att_state.blendEnable = VK_FALSE;
        blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        blend_state.attachmentCount = 1;
        blend_state.pAttachments = &blend_att_state;

        VkPipelineLayoutCreateInfo layout_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &dsl;
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(dev, &layout_info, nullptr, &layout); // maps shader resources with descriptors

        VkGraphicsPipelineCreateInfo pipeline_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_info.stageCount = shader_stage_info.size();
        pipeline_info.pStages = shader_stage_info.data();
        pipeline_info.pVertexInputState = &vert_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly_state;
        pipeline_info.pViewportState = &vp_state;
        pipeline_info.pRasterizationState = &rs_state;
        pipeline_info.pMultisampleState = &ms_state;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pColorBlendState = &blend_state;
        pipeline_info.pDynamicState = nullptr;
        pipeline_info.layout = layout;
        pipeline_info.renderPass = rp;
        pipeline_info.subpass = 0;
        vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
        vkDestroyShaderModule(dev, vert, nullptr);
        vkDestroyShaderModule(dev, frag, nullptr);
    }
    void BakeCommand() {
        for (uint32_t i = 0; i < cmdbuf.size(); i++) {
            VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(cmdbuf[i], &cmdbuf_begin_info);
            VkRenderPassBeginInfo begin_renderpass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            begin_renderpass_info.renderPass = rp;
            begin_renderpass_info.framebuffer = framebuffers[i];
            begin_renderpass_info.renderArea.extent = VkExtent2D { 800, 800 };
            begin_renderpass_info.renderArea.offset = VkOffset2D { 0, 0 };
            begin_renderpass_info.clearValueCount = 1;
            VkClearValue clear_value;
            clear_value.color = VkClearColorValue{ 0.01, 0.02, 0.03, 1.0 };
            begin_renderpass_info.pClearValues = &clear_value;
            vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkDeviceSize offsets[] { 0 };
            vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &interleaved_buf, offsets);
            vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &ds, 0, nullptr);
            vkCmdDraw(cmdbuf[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(cmdbuf[i]);
            vkEndCommandBuffer(cmdbuf[i]);
        }
    }
    void Gameloop() {
        uint32_t image_index;
        while (!glfwWindowShouldClose(window)) {
            // application / swapchain / presenter (display)
            // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
            vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &image_index);

            VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &image_available_sema; // application can't submit until it has an available image
            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuf[image_index]; // reference to prebuilt commandbuf
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &image_render_finished_sema; // GPU signals that it finishes rendering
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

            VkPresentInfoKHR present_info { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &image_render_finished_sema; // application can't present until it finished rendering
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &swapchain;
            present_info.pImageIndices = &image_index;
            vkQueuePresentKHR(queue, &present_info);
            glfwPollEvents();
        }
    }
private:
    GLFWwindow *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice pdev;
    uint32_t q_family_index;
    VkDevice dev;
    VkQueue queue;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_imageviews;
    VkSemaphore image_available_sema;
    VkSemaphore image_render_finished_sema;
    VkCommandPool cmdpool;
    std::vector<VkCommandBuffer> cmdbuf;
    VkRenderPass rp;
    std::vector<VkFramebuffer> framebuffers;
    VkBuffer interleaved_buf; // a single buffer holds all the vertex attributes
    VkDeviceMemory interleaved_bufmem;
    VkImage tex_image;
    VkDeviceMemory tex_mem;
    VkImageView tex_imageview;
    VkSampler sampler;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkDescriptorSetLayout dsl;
    VkDescriptorPool pool;
    VkDescriptorSet ds;
};

int main(int argc, char **argv) {
    HelloVulkan demo;
    demo.Gameloop();
    return 0;
}
