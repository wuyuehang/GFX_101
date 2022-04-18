#include "HelloVulkan.hpp"

void HelloVulkan::bake_wireframe_DescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo dsl_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    dsl_info.bindingCount = bindings.size();
    dsl_info.pBindings = bindings.data();
    vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &wireframe_pipe.dsl);
}

void HelloVulkan::bake_wireframe_DescriptorSet() {
    wireframe_pipe.ds.resize(m_max_inflight_frames);

    std::vector<VkDescriptorPoolSize> pool_size(1);
    pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size[0].descriptorCount = wireframe_pipe.ds.size();

    VkDescriptorPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = wireframe_pipe.ds.size();
    pool_info.poolSizeCount = pool_size.size();
    pool_info.pPoolSizes = pool_size.data();
    vkCreateDescriptorPool(dev, &pool_info, nullptr, &wireframe_pipe.pool);

    for (uint32_t i = 0; i < wireframe_pipe.ds.size(); i++) {
        VkDescriptorSetAllocateInfo alloc_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = wireframe_pipe.pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &wireframe_pipe.dsl;
        vkAllocateDescriptorSets(dev, &alloc_info, &wireframe_pipe.ds[i]);

        VkDescriptorBufferInfo buf_info {
            .buffer = uniform[i]->get_buffer(),
            .offset = 0,
            .range = sizeof(struct MVP),
        };

        VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = wireframe_pipe.ds[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &buf_info;

        vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);
    }
}

void HelloVulkan::bake_wireframe_Pipeline() {
    auto vert = loadSPIRV(dev, "wireframe.vert.spv");
    auto frag = loadSPIRV(dev, "wireframe.frag.spv");

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
    vbd[0].stride = 3 * sizeof(float);
    vbd[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> ad(1);
    ad[0].location = 0; // Position.xyz
    ad[0].binding = 0; // vertex buffer
    ad[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    ad[0].offset = 0;

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
        .polygonMode = VK_POLYGON_MODE_LINE, // wireframe mode
        .cullMode = VK_CULL_MODE_NONE,
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
    VkPipelineColorBlendStateCreateInfo blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = &blend_att_state;

    VkPipelineLayoutCreateInfo layout_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &wireframe_pipe.dsl;
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(dev, &layout_info, nullptr, &wireframe_pipe.pipeline_layout);

    VkGraphicsPipelineCreateInfo pipeline_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
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
    pipeline_info.layout = wireframe_pipe.pipeline_layout;
    pipeline_info.renderPass = rp;
    pipeline_info.subpass = 0;
    vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &wireframe_pipe.pipeline);
    vkDestroyShaderModule(dev, vert, nullptr);
    vkDestroyShaderModule(dev, frag, nullptr);
}