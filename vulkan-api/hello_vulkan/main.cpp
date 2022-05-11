#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class HelloVulkan {
public:
    HelloVulkan() {
        InitGLFW();
        CreateInstance();
        glfwCreateWindowSurface(instance, window, nullptr, &surface); // create a presentation surface
        CreateDevice();
        CreateSwapchain();
        InitSync();
        CreateRenderPass();
        CreateFramebuffer();
        CreatePipeline();
        BakeCommand();
    }
    ~HelloVulkan() {
        vkDeviceWaitIdle(dev);
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, layout, nullptr);
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
        window = glfwCreateWindow(800, 800, "HelloVulkan", nullptr, nullptr);
    }
    void CreateInstance() {
        // dynamically fetch vulkan function by dlopen libvulkan.
        // no need to include vulkan header, otherwise it will alias
        // #define vk_global_func( fun ) fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun)
        // vk_global_func( vkCreateInstance );
#ifdef DEBUG
        uint32_t api_version;
        vkEnumerateInstanceVersion(&api_version);
        assert(VK_VERSION_MAJOR(api_version) >= 1 && VK_VERSION_MINOR(api_version) >= 1);

        uint32_t instance_layer_count;
        vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
        std::vector<VkLayerProperties> instance_layer_properties(instance_layer_count);
        vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties.data());
        for (auto& it : instance_layer_properties) {
            std::cout << "layer name: " << it.layerName << std::endl;
            std::cout << "description: " << it.description << std::endl;
        }

        uint32_t instance_extension_count;
        vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
        std::vector<VkExtensionProperties> instance_extension_properties(instance_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extension_properties.data());
        for (auto& it : instance_extension_properties) {
            std::cout << "extension name: " << it.extensionName << std::endl;
        }
#endif
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = "HelloVulkan";
        app_info.pEngineName = "HelloVulkan";
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
#ifdef DEBUG
        uint32_t device_extension_count;
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &device_extension_count, nullptr);
        std::vector<VkExtensionProperties> device_extension_properties(device_extension_count);
        vkEnumerateDeviceExtensionProperties(pdev, nullptr, &device_extension_count, device_extension_properties.data());
        for (auto& it : device_extension_properties) {
            std::cout << "device extension name: " << it.extensionName << std::endl;
        }
#endif
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
        //chain_info.clipped =
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
    void BakeCommand() {
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

        for (uint32_t i = 0; i < cmdbuf.size(); i++) {
            VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vkBeginCommandBuffer(cmdbuf[i], &cmdbuf_begin_info);
#ifdef DEBUG
            VkImageSubresourceRange range {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            // --> VK_IMAGE_USAGE_TRANSFER_DST_BIT
            VkImageMemoryBarrier barrier_from_present_to_clear {};
            barrier_from_present_to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier_from_present_to_clear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier_from_present_to_clear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier_from_present_to_clear.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            barrier_from_present_to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier_from_present_to_clear.srcQueueFamilyIndex = q_family_index;
            barrier_from_present_to_clear.dstQueueFamilyIndex = q_family_index;
            barrier_from_present_to_clear.image = swapchain_images[i];
            barrier_from_present_to_clear.subresourceRange = range;
            vkCmdPipelineBarrier(cmdbuf[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_clear);
            VkClearColorValue clear_color {1.0, 0.0, 0.0, 1.0};
            vkCmdClearColorImage(cmdbuf[i], swapchain_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

            // --> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            VkImageMemoryBarrier barrier_from_clear_to_present {};
            barrier_from_clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier_from_clear_to_present.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            barrier_from_clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier_from_clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            barrier_from_clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier_from_clear_to_present.srcQueueFamilyIndex = q_family_index;
            barrier_from_clear_to_present.dstQueueFamilyIndex = q_family_index;
            barrier_from_clear_to_present.image = swapchain_images[i];
            barrier_from_clear_to_present.subresourceRange = range;
            vkCmdPipelineBarrier(cmdbuf[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_clear_to_present);
#else
            VkRenderPassBeginInfo begin_renderpass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            begin_renderpass_info.renderPass = rp;
            begin_renderpass_info.framebuffer = framebuffers[i];
            begin_renderpass_info.renderArea.extent = VkExtent2D { 800, 800 };
            begin_renderpass_info.renderArea.offset = VkOffset2D { 0, 0 };
            begin_renderpass_info.clearValueCount = 1;
            VkClearValue clear_value;
            clear_value.color = VkClearColorValue{ 0.1, 0.2, 0.3, 1.0 };
            begin_renderpass_info.pClearValues = &clear_value;
            vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdDraw(cmdbuf[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(cmdbuf[i]);
#endif
            vkEndCommandBuffer(cmdbuf[i]);
        }
    }
    void CreateRenderPass() {
        VkAttachmentDescription att_desc {};
        //att_desc.flags =
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
    void CreatePipeline() {
        // shaders related blobs
        auto vert_spv = loadSPIRV("simple.vert.spv");
        auto frag_spv = loadSPIRV("simple.frag.spv");
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
        VkPipelineVertexInputStateCreateInfo vert_input_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
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
        layout_info.setLayoutCount = 0;
        layout_info.pSetLayouts = nullptr;
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
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_imageviews;
    VkSemaphore image_available_sema;
    VkSemaphore image_render_finished_sema;
    VkCommandPool cmdpool;
    std::vector<VkCommandBuffer> cmdbuf;
    VkRenderPass rp;
    std::vector<VkFramebuffer> framebuffers;
    VkPipelineLayout layout;
    VkPipeline pipeline;
};

int main(int argc, char **argv) {
    HelloVulkan demo;
    demo.Gameloop();
    return 0;
}