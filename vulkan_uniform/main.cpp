#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define USE_GLFW_CALLBACK 0

#if USE_GLFW_CALLBACK
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
#endif

class HelloVulkan {
public:
    struct Camera {
        glm::vec3 eye;
        glm::vec3 front;
        glm::vec3 up;
        float fov;
        float ratio;
        float near;
        float far;
    };
    HelloVulkan() {
        InitGLFW();
        CreateInstance();
        glfwCreateWindowSurface(instance, window, nullptr, &surface); // create a presentation surface
        CreateDevice();
        CreateSwapchain();
        InitSync();
        InitDepth();
        CreateCommand();
        CreateRenderPass();
        CreateFramebuffer();
        CreateVertexBuffer();
        CreateUniform();
        CreatePipeline();
        BakeCommand();
    }
    ~HelloVulkan() {
        vkDeviceWaitIdle(dev);
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, layout, nullptr);
        for (const auto& it : uni_bufmem) {
            vkFreeMemory(dev, it, nullptr);
        }
        for (const auto& it : uni_buf) {
            vkDestroyBuffer(dev, it, nullptr);
        }
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, pool, ds.size(), ds.data());
        vkDestroyDescriptorPool(dev, pool, nullptr);
        vkFreeMemory(dev, index_bufmem, nullptr);
        vkDestroyBuffer(dev, index_buf, nullptr);
        vkFreeMemory(dev, interleaved_bufmem, nullptr);
        vkDestroyBuffer(dev, interleaved_buf, nullptr);

        for (auto& it : framebuffers) {
            vkDestroyFramebuffer(dev, it, nullptr);
        }
        vkDestroyRenderPass(dev, rp, nullptr);
        vkDestroyImageView(dev, depth_imgv, nullptr);
        vkFreeMemory(dev, depth_mem, nullptr);
        vkDestroyImage(dev, depth_img, nullptr);
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
        window = glfwCreateWindow(800, 800, "HelloVulkan", nullptr, nullptr);
#if USE_GLFW_CALLBACK
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, key_callback);
#endif
    }
    void CreateInstance() {
        // dynamically fetch vulkan function by dlopen libvulkan.
        // no need to include vulkan header, otherwise it will alias
        // #define vk_global_func( fun ) fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun)
        // vk_global_func( vkCreateInstance );
        VkApplicationInfo app_info {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "VertexAttribute";
        app_info.pEngineName = "VertexAttribute";
        app_info.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo instance_info {};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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

        VkDeviceQueueCreateInfo queue_info {};
        float q_prio = 1.0;
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = q_family_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &q_prio;

        VkDeviceCreateInfo dev_info {};
        dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        std::vector<const char *> dev_ext;
        dev_ext.push_back("VK_KHR_swapchain"); // device level extension to enable render onscreen images.
        //dev_ext.push_back("VK_KHR_create_renderpass2");
        //dev_ext.push_back("VK_KHR_separate_depth_stencil_layouts");
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

        VkSwapchainCreateInfoKHR chain_info {};
        chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //chain_info.flags =
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
        //chain_info.clipped =
        chain_info.oldSwapchain = VK_NULL_HANDLE;
        vkCreateSwapchainKHR(dev, &chain_info, nullptr, &swapchain);

        uint32_t swapchain_image_count;
        vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, nullptr);
        swapchain_images.resize(swapchain_image_count);
        vkGetSwapchainImagesKHR(dev, swapchain, &swapchain_image_count, swapchain_images.data());

        swapchain_imageviews.resize(swapchain_image_count);
        for (uint8_t i = 0; i < swapchain_images.size(); i++) {
            VkImageViewCreateInfo imageview_info {};
            imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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
        VkSemaphoreCreateInfo sema_info {};
        sema_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_available_sema);
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_render_finished_sema);
    }
    void CreateCommand() {
        VkCommandPoolCreateInfo commandpool_info {};
        commandpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow individual commandbuffer reset
        commandpool_info.queueFamilyIndex = q_family_index;
        vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

        VkCommandBufferAllocateInfo cmdbuf_info {};
        cmdbuf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbuf_info.commandPool = cmdpool;
        cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdbuf_info.commandBufferCount = swapchain_images.size(); // must be per image
        cmdbuf.resize(swapchain_images.size());
        vkAllocateCommandBuffers(dev, &cmdbuf_info, cmdbuf.data());
    }
    void InitDepth() {
        VkImageCreateInfo image_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = VK_FORMAT_D32_SFLOAT;
        image_info.extent = VkExtent3D {800, 800, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkCreateImage(dev, &image_info, nullptr, &depth_img);

        VkMemoryRequirements img_req {};
        vkGetImageMemoryRequirements(dev, depth_img, &img_req);

        VkMemoryAllocateInfo img_alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        img_alloc_info.allocationSize = img_req.size;
        img_alloc_info.memoryTypeIndex = findMemoryType(img_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(dev, &img_alloc_info, nullptr, &depth_mem);

        vkBindImageMemory(dev, depth_img, depth_mem, 0);

        VkImageViewCreateInfo view_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = depth_img;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_D32_SFLOAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        vkCreateImageView(dev, &view_info, nullptr, &depth_imgv);
    }
    void CreateRenderPass() {
        std::array<VkAttachmentDescription, 2> att_desc {};
        att_desc[0].format = VK_FORMAT_B8G8R8A8_SRGB; // same as swapchain
        att_desc[0].samples = VK_SAMPLE_COUNT_1_BIT;
        att_desc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att_desc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att_desc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att_desc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att_desc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image layout changes within a render pass witout image memory barrier
        att_desc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        att_desc[1].format = VK_FORMAT_D32_SFLOAT; // depth
        att_desc[1].samples = VK_SAMPLE_COUNT_1_BIT;
        att_desc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att_desc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att_desc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att_desc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att_desc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // If the separateDepthStencilLayouts feature is not enabled, finalLayout must not be VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, or VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL
        att_desc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentReference, 2> att_ref {};
        att_ref[0].attachment = 0;
        att_ref[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        att_ref[1].attachment = 1;
        att_ref[1].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        VkSubpassDescription sp_desc {};
        sp_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp_desc.colorAttachmentCount = 1;
        sp_desc.pColorAttachments = &att_ref[0];
        sp_desc.pDepthStencilAttachment = &att_ref[1];

        VkRenderPassCreateInfo rp_info {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rp_info.attachmentCount = att_desc.size();
        rp_info.pAttachments = att_desc.data();
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &sp_desc;

        vkCreateRenderPass(dev, &rp_info, nullptr, &rp);
    }
    void CreateFramebuffer() {
        // render pass doesn't specify what images to use
        framebuffers.resize(swapchain_images.size());
        for (uint32_t i = 0; i < swapchain_images.size(); i++) {
            std::array<VkImageView, 2> attachments { swapchain_imageviews[i], depth_imgv };
            VkFramebufferCreateInfo fb_info {};
            fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fb_info.renderPass = rp;
            fb_info.attachmentCount = attachments.size();
            fb_info.pAttachments = attachments.data(); // translate imageview(s) to attachment(s)
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
        {
            const std::vector<uint16_t> index_data { 0, 1, 2, 3, 4, 5, 6, 7, 8, /* x axis */ 9, 10, 11, /* y axis */ 12, 13, 14, /* z axis */ 15, 16, 17 };
            VkBufferCreateInfo ibuf_info {};
            ibuf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ibuf_info.size = index_data.size()*sizeof(uint16_t);
            ibuf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            ibuf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &ibuf_info, nullptr, &index_buf);

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, index_buf, &req);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &index_bufmem);

            vkBindBufferMemory(dev, index_buf, index_bufmem, 0);

            void *buf_ptr;
            vkMapMemory(dev, index_bufmem, 0, ibuf_info.size, 0, &buf_ptr);
            memcpy(buf_ptr, index_data.data(), ibuf_info.size);
            vkUnmapMemory(dev, index_bufmem);
        }
        const std::vector<float> interleaved_vb_data {
            -1.0, -1.0, 1.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            1.0, -1.0, 1.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            -1.0, 1.0, 1.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            -0.75, -0.75, 0.75, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
            0.75, -0.75, 0.75, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
            -0.75, 0.75, 0.75, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
            -0.5, -0.5, 0.5, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            0.5, -0.5, 0.5, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            -0.5, 0.5, 0.5, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            // local X axis
            0.0, 0.02, 0.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            0.0, -0.02, 0.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            0.2, -0.02, 0.0, 1.0, /* Position */ 1.0, 0.0, 0.0, 1.0, /* Color */
            // local y axis
            0.02, 0.0, 0.0, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            -0.02, 0.0, 0.0, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            -0.02, -0.2, 0.0, 1.0, /* Position */ 0.0, 1.0, 0.0, 1.0, /* Color */
            // local z axis
            0.0, -0.02, 0.0, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
            0.0, 0.02, 0.0, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
            0.0, -0.02, 0.2, 1.0, /* Position */ 0.0, 0.0, 1.0, 1.0, /* Color */
        };
        VkBuffer interleaved_buf_staging;
        VkDeviceMemory interleaved_bufmem_staging;
        VkBufferCreateInfo buf_info {};
        buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_info.size = interleaved_vb_data.size() * sizeof(float);
        buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(dev, &buf_info, nullptr, &interleaved_buf_staging); // CPU access friendly

        VkMemoryRequirements req {};
        vkGetBufferMemoryRequirements(dev, interleaved_buf_staging, &req);

        VkMemoryAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        vkAllocateMemory(dev, &alloc_info, nullptr, &interleaved_bufmem_staging);

        vkBindBufferMemory(dev, interleaved_buf_staging, interleaved_bufmem_staging, 0);

        void *buf_ptr;
        vkMapMemory(dev, interleaved_bufmem_staging, 0, buf_info.size, 0, &buf_ptr);
        memcpy(buf_ptr, interleaved_vb_data.data(), buf_info.size);
        vkUnmapMemory(dev, interleaved_bufmem_staging);
        {
            VkBufferCreateInfo buf_info {};
            buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_info.size = interleaved_vb_data.size() * sizeof(float);
            buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &buf_info, nullptr, &interleaved_buf); // GPU access friendly

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, interleaved_buf, &req);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &interleaved_bufmem);

            vkBindBufferMemory(dev, interleaved_buf, interleaved_bufmem, 0);
        }

        VkCommandBufferBeginInfo cmdbuf_begin_info {};
        cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(cmdbuf[0], &cmdbuf_begin_info);
        VkBufferCopy region { 0, 0, buf_info.size };
        vkCmdCopyBuffer(cmdbuf[0], interleaved_buf_staging, interleaved_buf, 1, &region);
        vkEndCommandBuffer(cmdbuf[0]);

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdbuf[0];

        VkFence fence;
        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(dev, &fence_info, nullptr, &fence);
        vkQueueSubmit(queue, 1, &submit_info, fence);
        vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(dev, fence, nullptr);
        vkFreeMemory(dev, interleaved_bufmem_staging, nullptr);
        vkDestroyBuffer(dev, interleaved_buf_staging, nullptr);
        vkResetCommandBuffer(cmdbuf[0], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }
    void CreateUniform() {
        struct UBO {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };
        UBO ubo {};
        ubo.model = glm::mat4(1.0f);

        uni_buf.resize(swapchain_images.size());
        uni_bufmem.resize(swapchain_images.size());

        for (uint32_t i = 0; i < uni_buf.size(); i++) {
            VkBufferCreateInfo unibuf_info {};
            unibuf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            unibuf_info.size = sizeof(ubo);
            unibuf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            unibuf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(dev, &unibuf_info, nullptr, &uni_buf[i]);

            VkMemoryRequirements req {};
            vkGetBufferMemoryRequirements(dev, uni_buf[i], &req);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(dev, &alloc_info, nullptr, &uni_bufmem[i]);

            vkBindBufferMemory(dev, uni_buf[i], uni_bufmem[i], 0);

            void *buf_ptr;
            vkMapMemory(dev, uni_bufmem[i], 0, unibuf_info.size, 0, &buf_ptr);
            memcpy(buf_ptr, &ubo, unibuf_info.size);
            vkUnmapMemory(dev, uni_bufmem[i]);
        }
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo dsl_info {};
        dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsl_info.bindingCount = bindings.size();
        dsl_info.pBindings = bindings.data();
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);

        ds.resize(swapchain_images.size());

        VkDescriptorPoolSize pool_size {};
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = ds.size();

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = ds.size();
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool);

        for (uint32_t i = 0; i < ds.size(); i++) {
            VkDescriptorSetAllocateInfo alloc_info {};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &dsl;
            vkAllocateDescriptorSets(dev, &alloc_info, &ds[i]);

            VkDescriptorBufferInfo buf_info {
                .buffer = uni_buf[i],
                .offset = 0,
                .range = sizeof(ubo),
            };

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = ds[i];
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &buf_info;

            vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);
        }
    }
    void CreatePipeline() {
        // shaders related blobs
        auto vert_spv = loadSPIRV("simple.vert.spv");
        auto frag_spv = loadSPIRV("simple.frag.spv");
        VkShaderModule vert, frag;
        VkShaderModuleCreateInfo shader_info {};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.codeSize = vert_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(vert_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &vert);
        shader_info.codeSize = frag_spv.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(frag_spv.data());
        vkCreateShaderModule(dev, &shader_info, nullptr, &frag);

        VkPipelineShaderStageCreateInfo vert_stage_info {};
        vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_stage_info.module = vert;
        vert_stage_info.pName = "main";
        VkPipelineShaderStageCreateInfo frag_stage_info {};
        frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_stage_info.module = frag;
        frag_stage_info.pName = "main";
        std::vector<VkPipelineShaderStageCreateInfo> shader_stage_info;
        shader_stage_info.clear();
        shader_stage_info.push_back(vert_stage_info);
        shader_stage_info.push_back(frag_stage_info);

        // vertex input
        std::vector<VkVertexInputBindingDescription> vbd(1);
        vbd[0].binding = 0;
        vbd[0].stride = 8*sizeof(float);
        vbd[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> ad(2);
        ad[0].location = 0; // Position
        ad[0].binding = 0; // single vertex attribute buffer
        ad[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        ad[0].offset = 0;
        ad[1].location = 1; // Color
        ad[1].binding = 0;
        ad[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
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

        // depth stencil
        VkPipelineDepthStencilStateCreateInfo ds_state { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds_state.depthTestEnable = VK_TRUE;
        ds_state.depthWriteEnable = VK_TRUE;
        ds_state.depthCompareOp = VK_COMPARE_OP_LESS;

        // color blend
        // if renderPass is not VK_NULL_HANDLE, the pipeline is being created with fragment output interface state,
        // and subpass uses color attachments, pColorBlendState must be a valid pointer to a valid
        // VkPipelineColorBlendStateCreateInfo structure.
        VkPipelineColorBlendAttachmentState blend_att_state {};
        blend_att_state.blendEnable = VK_FALSE;
        blend_att_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend_state {};
        blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        blend_state.attachmentCount = 1;
        blend_state.pAttachments = &blend_att_state;

        VkPipelineLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = &dsl;
        layout_info.pushConstantRangeCount = 0;
        layout_info.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(dev, &layout_info, nullptr, &layout); // maps shader resources with descriptors

        VkGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        //pipeline_info.flags =
        pipeline_info.stageCount = shader_stage_info.size();
        pipeline_info.pStages = shader_stage_info.data();
        pipeline_info.pVertexInputState = &vert_input_state;
        pipeline_info.pInputAssemblyState = &input_assembly_state;
        pipeline_info.pViewportState = &vp_state;
        pipeline_info.pRasterizationState = &rs_state;
        pipeline_info.pMultisampleState = &ms_state;
        pipeline_info.pDepthStencilState = &ds_state;
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
            VkCommandBufferBeginInfo cmdbuf_begin_info {};
            cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(cmdbuf[i], &cmdbuf_begin_info);
            VkRenderPassBeginInfo begin_renderpass_info {};
            begin_renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            begin_renderpass_info.renderPass = rp;
            begin_renderpass_info.framebuffer = framebuffers[i];
            begin_renderpass_info.renderArea.extent = VkExtent2D { 800, 800 };
            begin_renderpass_info.renderArea.offset = VkOffset2D { 0, 0 };
            std::array<VkClearValue, 2> clear_value {};
            clear_value[0].color = VkClearColorValue{ 0.01, 0.02, 0.03, 1.0 };
            clear_value[1].depthStencil.depth = 1.0;
            begin_renderpass_info.clearValueCount = clear_value.size();
            begin_renderpass_info.pClearValues = clear_value.data();
            vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindIndexBuffer(cmdbuf[i], index_buf, 0, VK_INDEX_TYPE_UINT16);
            VkDeviceSize offsets[] { 0 };
            vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &interleaved_buf, offsets);
            vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &ds[i], 0, nullptr);
            vkCmdDrawIndexed(cmdbuf[i], 18, 1, 0, 0, 0);
            vkCmdEndRenderPass(cmdbuf[i]);
            vkEndCommandBuffer(cmdbuf[i]);
        }
    }
    void HandleInput() {
        static bool init_control_system = false;
        static float curr_time, last_time;
        float delta;
        static double curr_pitch, curr_yaw;
        static double xpos, ypos, last_xpos, last_ypos;
        static bool start = false;

        if (!init_control_system) {
            camera.eye = glm::vec3(0.0, 0.0, 15.0);
            camera.up = glm::vec3(0.0, 1.0, 0.0);
            camera.front = glm::vec3(0.0, 0.0, -10000.0) - camera.eye;

            camera.fov = 45.0;
            camera.ratio = 1.0;
            camera.near = 0.1;
            camera.far = 100.0;

            curr_pitch = 0.0;
            curr_yaw = 180.0; // looking at negative Z axis
            xpos = 0.0;
            ypos = 0.0;
            last_xpos = 0.0;
            last_ypos = 0.0;
            start = false;

            curr_time = 0.0;
            last_time = 0.0;
            init_control_system = true;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        {
            curr_time = glfwGetTime();
            delta = curr_time - last_time;
            last_time = curr_time;
        }

        float cameraSpeed = 2.75 * delta;
        float mouseSpeed = 2.75 * delta;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glfwGetCursorPos(window, &xpos, &ypos);

            if (!start) { // when press first time, we update last x/y position only.
                last_xpos = xpos;
                last_ypos = ypos;
                start = true;
            } else { // when enter again, we calculate the yaw and pitch.
                curr_yaw += mouseSpeed * (xpos - last_xpos);
                curr_pitch += mouseSpeed * (ypos - last_ypos);
                last_xpos = xpos;
                last_ypos = ypos;
            }
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            start = false; // when release, we flag the ending.
        }

        camera.front = glm::vec3(
            cos(glm::radians(curr_pitch)) * sin(glm::radians(curr_yaw)),
            sin(glm::radians(curr_pitch)),
            cos(glm::radians(curr_pitch)) * cos(glm::radians(curr_yaw))
        );

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.eye += cameraSpeed * glm::normalize(camera.front);
        } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.eye -= cameraSpeed * glm::normalize(camera.front);
        } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.eye -= cameraSpeed * glm::normalize(glm::cross(camera.front, camera.up));
        } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.eye += cameraSpeed * glm::normalize(glm::cross(camera.front, camera.up));
        } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            init_control_system = false;
        }
    }
    void Gameloop() {
        uint32_t image_index;
        while (!glfwWindowShouldClose(window)) {
            // application / swapchain / presenter (display)
            // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
            vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &image_index);

            HandleInput();
            {
                struct UBO {
                    glm::mat4 model;
                    glm::mat4 view;
                    glm::mat4 proj;
                };
                UBO ubo {};
                ubo.model = glm::mat4(1.0f);
                ubo.view = glm::lookAt(camera.eye, camera.front + camera.eye, camera.up);
                ubo.proj = glm::perspective(glm::radians(camera.fov), camera.ratio, camera.near, camera.far);

                void *buf_ptr;
                vkMapMemory(dev, uni_bufmem[image_index], 0, sizeof(ubo), 0, &buf_ptr);
                memcpy(buf_ptr, &ubo, sizeof(ubo));
                vkUnmapMemory(dev, uni_bufmem[image_index]);
            }

            VkSubmitInfo submit_info {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &image_available_sema; // application can't submit until it has an available image
            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuf[image_index]; // reference to prebuilt commandbuf
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &image_render_finished_sema; // GPU signals that it finishes rendering
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

            VkPresentInfoKHR present_info {};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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
    VkBuffer index_buf;
    VkDeviceMemory index_bufmem;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    std::vector<VkBuffer> uni_buf;
    std::vector<VkDeviceMemory> uni_bufmem;
    VkDescriptorSetLayout dsl;
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> ds;
    VkImage depth_img;
    VkImageView depth_imgv;
    VkDeviceMemory depth_mem;
public:
    struct Camera camera;
};

#if USE_GLFW_CALLBACK
// It only illustrates how to pass class object pointer to glfw callback
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    HelloVulkan *hello_vk = static_cast<HelloVulkan*>(glfwGetWindowUserPointer(window));
    struct HelloVulkan::Camera & camera = hello_vk->camera;
    static float curr_time = 0.0;
    static float last_time = 0.0;

    curr_time = glfwGetTime();
    float delta = curr_time - last_time;
    last_time = curr_time;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    float cameraSpeed = 1.5;// * delta;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.eye += cameraSpeed * glm::normalize(camera.target - camera.eye);
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.eye -= cameraSpeed * glm::normalize(camera.target - camera.eye);
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.eye -= cameraSpeed * glm::normalize(glm::cross(camera.target - camera.eye, camera.up));
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.eye += cameraSpeed * glm::normalize(glm::cross(camera.target - camera.eye, camera.up));
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.eye = glm::vec3(0.0, 0.0, 5.0);
        camera.target = glm::vec3(0.0);
        camera.up = glm::vec3(0.0, 1.0, 0.0);
    }
}
#endif

int main(int argc, char **argv) {
    HelloVulkan demo;
    demo.Gameloop();
    return 0;
}
