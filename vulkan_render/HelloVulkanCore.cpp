#include "HelloVulkan.hpp"

void HelloVulkan::InitGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    w = 1280;
    h = 720;
    window = glfwCreateWindow(w, h, APP_NAME, nullptr, nullptr);
}

void HelloVulkan::CreateInstance() {
    VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = APP_NAME;
    app_info.pEngineName = APP_NAME;
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    std::array<const char *, 2> instance_extension { "VK_KHR_surface", "VK_KHR_xcb_surface" };
    instance_info.ppEnabledExtensionNames = instance_extension.data();
    instance_info.enabledExtensionCount = instance_extension.size();
    vkCreateInstance(&instance_info, nullptr, &instance);
}

void HelloVulkan::CreateDevice() {
    uint32_t pdev_count;
    vkEnumeratePhysicalDevices(instance, &pdev_count, nullptr);
    std::vector<VkPhysicalDevice> physical_dev(pdev_count);
    vkEnumeratePhysicalDevices(instance, &pdev_count, physical_dev.data());
    pdev = physical_dev[0];

    uint32_t queue_prop_count;
    q_family_index = 0;
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
    std::array<const char *, 1> dev_ext { "VK_KHR_swapchain" };
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;
    dev_info.enabledExtensionCount = dev_ext.size();
    dev_info.ppEnabledExtensionNames = dev_ext.data();

    VkPhysicalDeviceFeatures device_feature {};
    // polygonMode cannot be VK_POLYGON_MODE_POINT or VK_POLYGON_MODE_LINE if VkPhysicalDeviceFeatures->fillModeNonSolid is false.
    device_feature.fillModeNonSolid = VK_TRUE;
    // Enable geometry shader
    device_feature.geometryShader = VK_TRUE;
    dev_info.pEnabledFeatures = &device_feature;

    vkCreateDevice(pdev, &dev_info, nullptr, &dev);
    vkGetDeviceQueue(dev, q_family_index, 0, &queue);

    vkGetPhysicalDeviceMemoryProperties(pdev, &mem_properties);
}

void HelloVulkan::InitSync() {
    m_max_inflight_frames = vulkan_swapchain.images.size();
    VkSemaphoreCreateInfo sema_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // mark as signed upon creation

    image_available.resize(m_max_inflight_frames);
    image_render_finished.resize(m_max_inflight_frames);
    fence.resize(m_max_inflight_frames);

    for (auto i = 0; i < m_max_inflight_frames; i++) {
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_available[i]);
        vkCreateSemaphore(dev, &sema_info, nullptr, &image_render_finished[i]);
        vkCreateFence(dev, &fence_info, nullptr, &fence[i]);
    }
}

void HelloVulkan::clean_VulkanPipe(VulkanPipe p) {
    vkDestroyPipeline(dev, p.pipeline, nullptr);
    vkDestroyPipelineLayout(dev, p.pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(dev, p.dsl, nullptr);
    vkFreeDescriptorSets(dev, p.pool, p.ds.size(), p.ds.data());
    vkDestroyDescriptorPool(dev, p.pool, nullptr);
}

void HelloVulkan::begin_command_buffer(VkCommandBuffer &cmd) {
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(cmd, &cmdbuf_begin_info);
}

void HelloVulkan::end_command_buffer(VkCommandBuffer &cmd) {
    vkEndCommandBuffer(cmd);
}

void HelloVulkan::CreateCommand() {
    assert(m_max_inflight_frames != 0);
    VkCommandPoolCreateInfo commandpool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandpool_info.queueFamilyIndex = q_family_index;
    vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

    cmdbuf.resize(m_max_inflight_frames);
    VkCommandBufferAllocateInfo cmdbuf_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdbuf_info.commandPool = cmdpool;
    cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdbuf_info.commandBufferCount = m_max_inflight_frames;
    vkAllocateCommandBuffers(dev, &cmdbuf_info, cmdbuf.data());

    cmdbuf_info.commandPool = cmdpool;
    cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdbuf_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(dev, &cmdbuf_info, &transfer_cmdbuf);
}

void HelloVulkan::CreateRenderPass() {
    std::array<VkAttachmentDescription, 2> att_desc {};
    att_desc[0].format = VK_FORMAT_B8G8R8A8_SRGB; // same as swapchain
    att_desc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    att_desc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att_desc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att_desc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att_desc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att_desc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image layout changes within a render pass witout image memory barrier
    att_desc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    att_desc[1].format = VK_FORMAT_D24_UNORM_S8_UINT; // depth
    att_desc[1].samples = VK_SAMPLE_COUNT_1_BIT;
    att_desc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att_desc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att_desc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att_desc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att_desc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att_desc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 2> att_ref {};
    att_ref[0].attachment = 0;
    att_ref[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    att_ref[1].attachment = 1;
    att_ref[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sp_desc {};
    sp_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp_desc.colorAttachmentCount = 1;
    sp_desc.pColorAttachments = &att_ref[0];
    sp_desc.pDepthStencilAttachment = &att_ref[1];

    VkRenderPassCreateInfo rp_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rp_info.attachmentCount = att_desc.size();
    rp_info.pAttachments = att_desc.data();
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &sp_desc;

    vkCreateRenderPass(dev, &rp_info, nullptr, &rp);
}

void HelloVulkan::CreateFramebuffer() {
    // render pass doesn't specify what images to use
    framebuffers.resize(vulkan_swapchain.images.size());
    for (uint32_t i = 0; i < vulkan_swapchain.images.size(); i++) {
        std::array<VkImageView, 2> attachments { vulkan_swapchain.views[i], depth->get_image_view() };
        VkFramebufferCreateInfo fb_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fb_info.renderPass = rp;
        fb_info.attachmentCount = attachments.size();
        fb_info.pAttachments = attachments.data(); // translate imageview(s) to attachment(s)
        fb_info.width = w;
        fb_info.height = h;
        fb_info.layers = 1;
        vkCreateFramebuffer(dev, &fb_info, nullptr, &framebuffers[i]);
    }
}

void HelloVulkan::bake_imgui() {
    // Delegated descriptor set pool for ImplVulkan backend
    uint32_t dummy_size = 1000;
    VkDescriptorPoolSize pool_size[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, dummy_size },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, dummy_size },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, dummy_size },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, dummy_size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, dummy_size },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, dummy_size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, dummy_size },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, dummy_size },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dummy_size },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, dummy_size },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, dummy_size },
    };

    VkDescriptorPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = dummy_size * IM_ARRAYSIZE(pool_size);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_size);
    pool_info.pPoolSizes = pool_size;

    vkCreateDescriptorPool(dev, &pool_info, nullptr, &imgui_pool);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    assert(m_max_inflight_frames != 0);

    // Wire to ImGui
    ImGui_ImplVulkan_InitInfo init_info {
        .Instance = instance,
        .PhysicalDevice = pdev,
        .Device = dev,
        .QueueFamily = q_family_index,
        .Queue = queue,
        .PipelineCache = VK_NULL_HANDLE,
        .DescriptorPool = imgui_pool,
        .Subpass = 0,
        .MinImageCount = m_max_inflight_frames,
        .ImageCount = m_max_inflight_frames,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = nullptr,
        .CheckVkResultFn = nullptr,
    };

    ImGui_ImplVulkan_Init(&init_info, rp); // reuse default renderpass

    // Upload ImGui fonts to GPU
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(transfer_cmdbuf, &cmdbuf_begin_info);

    ImGui_ImplVulkan_CreateFontsTexture(transfer_cmdbuf);

    vkEndCommandBuffer(transfer_cmdbuf);

    VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &transfer_cmdbuf;

    VkFence fence;
    VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(dev, &fence_info, nullptr, &fence);
    vkQueueSubmit(queue, 1, &submit_info, fence);
    vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(dev, fence, nullptr);
    vkResetCommandBuffer(transfer_cmdbuf, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

int main(int argc, char **argv) {
    HelloVulkan demo;
    demo.Gameloop();
    return 0;
}