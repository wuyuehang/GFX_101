#include <cstring>
#include "HelloVulkan.hpp"

#if 0
HelloVulkan::HelloVulkan() : vulkan_swapchain(this), ctrl(new FPSController(this)) {
#else
HelloVulkan::HelloVulkan() : vulkan_swapchain(this), ctrl(new TrackballController(this)), m_current_frame(0) {
#endif
    InitGLFW();
    CreateInstance();
    CreateDevice();
    vulkan_swapchain.init();
    InitSync();
    {
        depth = new ImageObj(this);
        depth->init(VkExtent3D { static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 }, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }
    CreateCommand();
    CreateRenderPass();
    CreateFramebuffer();
    CreateResource();
    {
        bake_default_DescriptorSetLayout();
        bake_default_DescriptorSet();
        bake_default_Pipeline();
    }
    {
        bake_wireframe_DescriptorSetLayout();
        bake_wireframe_DescriptorSet();
        bake_wireframe_Pipeline();
    }
}

HelloVulkan::~HelloVulkan() {
    vkDeviceWaitIdle(dev);
    clean_VulkanPipe(wireframe_pipe);
    clean_VulkanPipe(default_pipe);

    for (const auto& it : uniform) {
        vkFreeMemory(dev, it->get_memory(), nullptr);
        vkDestroyBuffer(dev, it->get_buffer(), nullptr);
    }

    delete index;
    delete vertex;

    for (auto& it : framebuffers) {
        vkDestroyFramebuffer(dev, it, nullptr);
    }
    vkDestroyRenderPass(dev, rp, nullptr);
    delete depth;
    vkDestroySampler(dev, sampler, nullptr);
    delete tex;
    vkFreeCommandBuffers(dev, cmdpool, 1, &transfer_cmdbuf);
    vkFreeCommandBuffers(dev, cmdpool, cmdbuf.size(), cmdbuf.data());
    vkDestroyCommandPool(dev, cmdpool, nullptr);
    for (auto i = 0; i < m_max_inflight_frames; i++) {
        vkDestroySemaphore(dev, image_available[i], nullptr);
        vkDestroySemaphore(dev, image_render_finished[i], nullptr);
        vkDestroyFence(dev, fence[i], nullptr);
    }
    vulkan_swapchain.deinit();
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    delete ctrl;
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

void HelloVulkan::CreateResource() {
    // VERTEX
    const std::vector<uint16_t> index_data { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

    index = new BufferObj(this);
    index->init(index_data.size() * sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    index->upload(index_data.data(), index_data.size()*sizeof(uint16_t));

    const std::vector<float> interleaved_vb_data {
        -1.0, -1.0, 0.5, 1.0, /* Position */ 0.0, 0.0, /* UV */
        1.0, -1.0, 0.5, 1.0, /* Position */ 1.0, 0.0,
        -1.0, 1.0, 0.5, 1.0, /* Position */ 0.0, 1.0,
        -0.75, -0.75, 0.75, 1.0, /* Position */ 0.0, 0.0,
        0.75, -0.75, 0.75, 1.0, /* Position */ 1.0, 0.0,
        -0.75, 0.75, 0.75, 1.0, /* Position */ 0.0, 1.0,
        -0.5, -0.5, 1.0, 1.0, /* Position */ 0.0, 0.0,
        0.5, -0.5, 1.0, 1.0, /* Position */ 1.0, 0.0,
        -0.5, 0.5, 1.0, 1.0, /* Position */ 0.0, 1.0,
    };

    vertex = new BufferObj(this);
    vertex->init(interleaved_vb_data.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertex->upload(interleaved_vb_data.data(), interleaved_vb_data.size() * sizeof(float));

    // UNIFORM
    uniform.resize(m_max_inflight_frames);

    for (uint32_t i = 0; i < uniform.size(); i++) {
        uniform[i] = new BufferObj(this);
        uniform[i]->init(sizeof(struct MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    // TEXTURE
    tex = new ImageObj(this);
    tex->bake("./assets/wall.png");
    tex->transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void HelloVulkan::BakeCommand(uint32_t frame_nr) {
    VkCommandBufferBeginInfo cmdbuf_begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(cmdbuf[frame_nr], &cmdbuf_begin_info);

    VkRenderPassBeginInfo begin_renderpass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    begin_renderpass_info.renderPass = rp;
    begin_renderpass_info.framebuffer = framebuffers[frame_nr];
    begin_renderpass_info.renderArea.extent = VkExtent2D { (uint32_t)w, (uint32_t)h };
    begin_renderpass_info.renderArea.offset = VkOffset2D { 0, 0 };

    std::array<VkClearValue, 2> clear_value {};
    clear_value[0].color = VkClearColorValue{ 0.0, 0.0, 0.0, 1.0 };
    clear_value[1].depthStencil.depth = 1.0;
    begin_renderpass_info.clearValueCount = clear_value.size();
    begin_renderpass_info.pClearValues = clear_value.data();

    vkCmdBeginRenderPass(cmdbuf[frame_nr], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdbuf[frame_nr], VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipe.pipeline);
    vkCmdBindIndexBuffer(cmdbuf[frame_nr], index->get_buffer(), 0, VK_INDEX_TYPE_UINT16);
    VkDeviceSize offsets[] { 0 };
    vkCmdBindVertexBuffers(cmdbuf[frame_nr], 0, 1, &vertex->get_buffer(), offsets);
    vkCmdBindDescriptorSets(cmdbuf[frame_nr], VK_PIPELINE_BIND_POINT_GRAPHICS, default_pipe.pipeline_layout, 0, 1, &default_pipe.ds[frame_nr], 0, nullptr);
    vkCmdDrawIndexed(cmdbuf[frame_nr], 9, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmdbuf[frame_nr]);
    vkEndCommandBuffer(cmdbuf[frame_nr]);
}

void HelloVulkan::Gameloop() {
    while (!glfwWindowShouldClose(window)) {
        vkWaitForFences(dev, 1, &fence[m_current_frame], VK_TRUE, UINT64_MAX);

        // application / swapchain / presenter (display)
        // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
        uint32_t image_index;
        vkAcquireNextImageKHR(dev, vulkan_swapchain.swapchain, UINT64_MAX, image_available[m_current_frame], VK_NULL_HANDLE, &image_index);

        ctrl->handle_input();
        {
            MVP ubo {};
            ubo.model = ctrl->get_model();
            ubo.view = ctrl->get_view();
            ubo.proj = ctrl->get_proj();

            void *buf_ptr;
            vkMapMemory(dev, uniform[image_index]->get_memory(), 0, sizeof(ubo), 0, &buf_ptr);
            memcpy(buf_ptr, &ubo, sizeof(ubo));
            vkUnmapMemory(dev, uniform[image_index]->get_memory());
        }
        BakeCommand(m_current_frame);

        vkResetFences(dev, 1, &fence[m_current_frame]);

        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_available[m_current_frame]; // application can't submit until it has an available image
        VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdbuf[image_index]; // reference to prebuilt commandbuf
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &image_render_finished[m_current_frame]; // GPU signals that it finishes rendering
        vkQueueSubmit(queue, 1, &submit_info, fence[m_current_frame]); // to signal CPU (host) that it finishes rendering, so that command buffer can be rebuilt.

        VkPresentInfoKHR present_info { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &image_render_finished[m_current_frame]; // application can't present until it finished rendering
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &vulkan_swapchain.swapchain;
        present_info.pImageIndices = &image_index;
        vkQueuePresentKHR(queue, &present_info);

        m_current_frame = (m_current_frame + 1) % m_max_inflight_frames;
        glfwPollEvents();
    }
}