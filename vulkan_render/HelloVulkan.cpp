#include <cstring>
#include "HelloVulkan.hpp"

#if 0
HelloVulkan::HelloVulkan() : vulkan_swapchain(this), ctrl(new FPSController(this)) {
#else
HelloVulkan::HelloVulkan() : vulkan_swapchain(this), ctrl(new TrackballController(this)) {
#endif
    InitGLFW();
    CreateInstance();
    CreateDevice();
    InitSync();
    vulkan_swapchain.init();
    depth = new ImageObj(this);
    depth->init(VkExtent3D { static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 }, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    CreateCommand();
    CreateRenderPass();
    CreateFramebuffer();
    CreateResource();
    CreateDescriptorSetLayout();
    CreateDescriptorSet();
    CreatePipeline();
    BakeCommand();
}

HelloVulkan::~HelloVulkan() {
    vkDeviceWaitIdle(dev);
    vkDestroyPipeline(dev, pipeline, nullptr);
    vkDestroyPipelineLayout(dev, layout, nullptr);

    for (const auto& it : uniform) {
        vkFreeMemory(dev, it->get_memory(), nullptr);
        vkDestroyBuffer(dev, it->get_buffer(), nullptr);
    }

    delete index;
    delete vertex;
    vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
    vkFreeDescriptorSets(dev, pool, ds.size(), ds.data());
    vkDestroyDescriptorPool(dev, pool, nullptr);

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
    vkDestroySemaphore(dev, image_available_sema, nullptr);
    vkDestroySemaphore(dev, image_render_finished_sema, nullptr);
    vulkan_swapchain.deinit();
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    delete ctrl;
}

void HelloVulkan::CreateCommand() {
    VkCommandPoolCreateInfo commandpool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    commandpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandpool_info.queueFamilyIndex = q_family_index;
    vkCreateCommandPool(dev, &commandpool_info, nullptr, &cmdpool);

    VkCommandBufferAllocateInfo cmdbuf_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdbuf_info.commandPool = cmdpool;
    cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdbuf_info.commandBufferCount = vulkan_swapchain.images.size();
    cmdbuf.resize(vulkan_swapchain.images.size());
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
    uniform.resize(vulkan_swapchain.images.size());

    for (uint32_t i = 0; i < uniform.size(); i++) {
        uniform[i] = new BufferObj(this);
        uniform[i]->init(sizeof(struct MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    // TEXTURE
    tex = new ImageObj(this);
    tex->bake("./assets/wall.png");
    tex->transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void HelloVulkan::CreateDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dsl_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    dsl_info.bindingCount = bindings.size();
    dsl_info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);
}

void HelloVulkan::CreateDescriptorSet() {
    ds.resize(vulkan_swapchain.images.size());

    std::vector<VkDescriptorPoolSize> pool_size(2);
    pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size[0].descriptorCount = ds.size();

    pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size[1].descriptorCount = ds.size(); // Even sampler doesn't change over frames, still need reserve the same number as DescriptorSet is per frame.

    VkDescriptorPoolCreateInfo pool_info {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = ds.size();
    pool_info.poolSizeCount = pool_size.size();
    pool_info.pPoolSizes = pool_size.data();
    vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool);

    for (uint32_t i = 0; i < ds.size(); i++) {
        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &dsl;
        vkAllocateDescriptorSets(dev, &alloc_info, &ds[i]);

        VkDescriptorBufferInfo buf_info {
            .buffer = uniform[i]->get_buffer(),
            .offset = 0,
            .range = sizeof(struct MVP),
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

    VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    vkCreateSampler(dev, &sampler_info, nullptr, &sampler);
    VkDescriptorImageInfo img_info {
        .sampler = sampler,
        .imageView = tex->get_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    for (uint32_t i = 0; i < ds.size(); i++) {
        std::vector<VkWriteDescriptorSet> wds(1);
        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = ds[i];
        wds[0].dstBinding = 1;
        wds[0].dstArrayElement = 0;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        wds[0].pImageInfo = &img_info;
        vkUpdateDescriptorSets(dev, wds.size(), wds.data(), 0, nullptr);
    }
}

void HelloVulkan::CreatePipeline() {
    auto vert = loadSPIRV(dev, "simple.vert.spv");
    auto frag = loadSPIRV(dev, "combined_image_sampler.frag.spv");

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
        .width = (float)w,
        .height = (float)h,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };
    VkRect2D scissor {};
    scissor.offset = VkOffset2D { 0, 0 };
    scissor.extent = VkExtent2D { (uint32_t)w, (uint32_t)h };
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

void HelloVulkan::BakeCommand() {
    for (uint32_t i = 0; i < cmdbuf.size(); i++) {
        VkCommandBufferBeginInfo cmdbuf_begin_info {};
        cmdbuf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbuf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(cmdbuf[i], &cmdbuf_begin_info);
        VkRenderPassBeginInfo begin_renderpass_info {};
        begin_renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_renderpass_info.renderPass = rp;
        begin_renderpass_info.framebuffer = framebuffers[i];
        begin_renderpass_info.renderArea.extent = VkExtent2D { (uint32_t)w, (uint32_t)h };
        begin_renderpass_info.renderArea.offset = VkOffset2D { 0, 0 };
        std::array<VkClearValue, 2> clear_value {};
        clear_value[0].color = VkClearColorValue{ 0.01, 0.02, 0.03, 1.0 };
        clear_value[1].depthStencil.depth = 1.0;
        begin_renderpass_info.clearValueCount = clear_value.size();
        begin_renderpass_info.pClearValues = clear_value.data();
        vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindIndexBuffer(cmdbuf[i], index->get_buffer(), 0, VK_INDEX_TYPE_UINT16);
        VkDeviceSize offsets[] { 0 };
        vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &vertex->get_buffer(), offsets);
        vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &ds[i], 0, nullptr);
        vkCmdDrawIndexed(cmdbuf[i], 9, 1, 0, 0, 0);
        vkCmdEndRenderPass(cmdbuf[i]);
        vkEndCommandBuffer(cmdbuf[i]);
    }
}

void HelloVulkan::Gameloop() {
    uint32_t image_index;
    while (!glfwWindowShouldClose(window)) {
        // application / swapchain / presenter (display)
        // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
        vkAcquireNextImageKHR(dev, vulkan_swapchain.swapchain, UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &image_index);

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
        present_info.pSwapchains = &vulkan_swapchain.swapchain;
        present_info.pImageIndices = &image_index;
        vkQueuePresentKHR(queue, &present_info);
        glfwPollEvents();
    }
}