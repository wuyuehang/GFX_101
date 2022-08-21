#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonVulkan.hpp"

#define USE_SEPARATE_IMAGE_SAMPLER 0

namespace common {

class TestSubpass : public SkeletonVulkan {
public:
    TestSubpass() : SkeletonVulkan(VK_API_VERSION_1_0, 800, 800) {
        SkeletonVulkan::init();
        {
            VkSemaphoreCreateInfo semaInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            vkCreateSemaphore(dev, &semaInfo, nullptr, &image_available_sema);
            vkCreateSemaphore(dev, &semaInfo, nullptr, &image_render_finished_sema);
        }
        CreateBuffer();
        CreateTexture();
        CreateDescriptor();
        CreateRenderPass();
        CreateFramebuffer();
        CreatePipeline();
        BakeCommand();
    }
    ~TestSubpass() {
        vkDeviceWaitIdle(dev);
        delete vertex_buf; delete src_texture; delete dst_texture;
        vkDestroySampler(dev, sampler, nullptr);
        vkDestroySemaphore(dev, image_available_sema, nullptr);
        vkDestroySemaphore(dev, image_render_finished_sema, nullptr);
        for (auto & it : pipeline) {
            vkDestroyPipeline(dev, it, nullptr);
        }
        vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(dev, dsl, nullptr);
        vkFreeDescriptorSets(dev, ds_pool, ds.size(), ds.data());
        vkDestroyDescriptorPool(dev, ds_pool, nullptr);
        for (auto & it : framebuffers) {
            vkDestroyFramebuffer(dev, it, nullptr);
        }
        vkDestroyRenderPass(dev, rp, nullptr);
        vkFreeCommandBuffers(dev, cmdpool, cmdbuf.size(), cmdbuf.data());
    }
    void CreateBuffer() {
        const std::vector<float> vertex_data {
            -1.0, -1.0, 0.0, 1.0, /* Position */ 0.0, 0.0, /* Coordinate */
            1.0, -1.0, 0.0, 1.0, /* Position */ 1.0, 0.0, /* Coordinate */
            -1.0, 1.0, 0.0, 1.0, /* Position */ 0.0, 1.0, /* Cooridnate */
            -1.0, 1.0, 0.0, 1.0, /* Position */ 0.0, 1.0, /* Cooridnate */
            1.0, -1.0, 0.0, 1.0, /* Position */ 1.0, 0.0, /* Coordinate */
            1.0, 1.0, 0.0, 1.0, /* Position */ 1.0, 1.0, /* Coordinate */
        };
        vertex_buf = new GfxBuffer(this);
        vertex_buf->init(sizeof(float)*vertex_data.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vertex_buf->device_upload(vertex_data.data(), sizeof(float)*vertex_data.size());
    }
    void CreateTexture() {
        VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        vkCreateSampler(dev, &sampler_info, nullptr, &sampler);

        src_texture = new GfxImage2D(this);
        src_texture->bake("../resource/buu.png", VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        src_texture->transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        src_texture->set_descriptor_info(sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        dst_texture = new GfxImage2D(this);
        dst_texture->init(VkExtent3D { src_texture->get_width(), src_texture->get_height(), 1}, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        dst_texture->set_descriptor_info(sampler, VK_IMAGE_LAYOUT_GENERAL);
    }
    void CreateDescriptor() {
        // descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            GfxDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        };
        VkDescriptorSetLayoutCreateInfo dsl_info = GfxDescriptorSetLayoutCreateInfo(bindings);
        vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl);

        // pipeline layout
        VkPipelineLayoutCreateInfo pipeline_layout_info = GfxPipelineLayoutCreateInfo(&dsl, 1);
        vkCreatePipelineLayout(dev, &pipeline_layout_info, nullptr, &pipeline_layout); // Subpass 0 & Subpass 1 share the same pipeline layout

        // descriptor pool
        std::vector<VkDescriptorPoolSize> pool_size = {
            GfxDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2) // Subpass 0 & Subpass 1 use two individual descriptors
        };
        VkDescriptorPoolCreateInfo pool_info = GfxDescriptorPoolCreateInfo(2, pool_size); // Subpass 0 & Subpass 1 use two individual descriptor sets
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &ds_pool);

        // descriptor
        std::array<VkDescriptorSetLayout, 2> all_dsl = { dsl, dsl }; // If allocates multiple descriptor sets, the descriptor set layout need to be arrays as well.
        VkDescriptorSetAllocateInfo alloc_info = GfxDescriptorSetAllocateInfo(ds_pool, all_dsl.data(), all_dsl.size());
        vkAllocateDescriptorSets(dev, &alloc_info, ds.data());

        // update descriptor set for Subpass 0
        {
            std::vector<VkWriteDescriptorSet> w = {
                GfxWriteDescriptorSet(ds[0], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &src_texture->get_descriptor_info())
            };
            vkUpdateDescriptorSets(dev, w.size(), w.data(), 0, nullptr);
        }
        // update descriptor set for Subpass 1
        {
            std::vector<VkWriteDescriptorSet> w = {
                GfxWriteDescriptorSet(ds[1], 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &dst_texture->get_descriptor_info())
            };
            vkUpdateDescriptorSets(dev, w.size(), w.data(), 0, nullptr);
        }
    }
    void BakeCommand() {
        VkCommandBufferAllocateInfo cmdbuf_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cmdbuf_info.commandPool = cmdpool;
        cmdbuf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdbuf_info.commandBufferCount = swapchain->get_swapchain_images().size();
        cmdbuf.resize(swapchain->get_swapchain_images().size());
        vkAllocateCommandBuffers(dev, &cmdbuf_info, cmdbuf.data());

        for (uint32_t i = 0; i < cmdbuf.size(); i++) {
            begin_cmdbuf(cmdbuf[i]);
            std::vector<VkClearValue> clear_value(2);
            clear_value[0].color = VkClearColorValue{ 0.0, 0.0, 0.0, 1.0 };
            clear_value[1].color = VkClearColorValue{ 0.0, 0.0, 0.0, 1.0 };
            VkRenderPassBeginInfo begin_renderpass_info {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = rp,
                .framebuffer = framebuffers[i],
                .renderArea = VkRect2D {
                    .offset = VkOffset2D { 0, 0 },
                    .extent = VkExtent2D { 800, 800 },
                },
                .clearValueCount = clear_value.size(),
                .pClearValues = clear_value.data(),
            };
            vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);

            // Subpass 0
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline[0]);
            VkDeviceSize offsets[] { 0 };
            vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &vertex_buf->get_buffer(), offsets);
            vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &ds[0], 0, nullptr);
            vkCmdDraw(cmdbuf[i], 6, 1, 0, 0);
            // Subpass 1
            vkCmdNextSubpass(cmdbuf[i], VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline[1]);
            vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &vertex_buf->get_buffer(), offsets);
            vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &ds[1], 0, nullptr);
            vkCmdDraw(cmdbuf[i], 6, 1, 0, 0);

            vkCmdEndRenderPass(cmdbuf[i]);
            end_cmdbuf(cmdbuf[i]);
        }
    }
    void CreateRenderPass() {
        std::array<VkAttachmentDescription, 2> att_desc = {
            VkAttachmentDescription {
                .flags = 0,
                .format = VK_FORMAT_R8G8B8A8_SRGB, // Subpass 0 color render target (dst_texture)
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_GENERAL
            },
            VkAttachmentDescription {
                .flags = 0,
                .format = VK_FORMAT_B8G8R8A8_SRGB,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        };

        std::array<VkAttachmentReference, 2> att_ref {
            VkAttachmentReference {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_GENERAL
            },
            VkAttachmentReference {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        };

        std::vector<VkSubpassDescription> sp_desc(2);
        sp_desc[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp_desc[0].colorAttachmentCount = 1;
        sp_desc[0].pColorAttachments = &att_ref[0];

        sp_desc[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp_desc[1].colorAttachmentCount = 1;
        sp_desc[1].pColorAttachments = &att_ref[1];

        std::vector<VkSubpassDependency> deps(3);
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[0].srcAccessMask;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        deps[1].srcSubpass = 0;
        deps[1].dstSubpass = 1;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        deps[2].srcSubpass = 1;
        deps[2].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        deps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[2].dstAccessMask;
        deps[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rp_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rp_info.attachmentCount = att_desc.size();
        rp_info.pAttachments = att_desc.data();
        rp_info.subpassCount = sp_desc.size();
        rp_info.pSubpasses = sp_desc.data();
        rp_info.dependencyCount = deps.size();
        rp_info.pDependencies = deps.data();

        vkCreateRenderPass(dev, &rp_info, nullptr, &rp);
    }
    void CreateFramebuffer() {
        framebuffers.resize(swapchain->get_swapchain_images().size());
        auto swapchain_views = swapchain->get_swapchain_views();
        for (uint32_t i = 0; i < framebuffers.size(); i++) {
            std::array<VkImageView, 2> att { dst_texture->get_image_view(), swapchain_views[i] };
            VkFramebufferCreateInfo fb_info {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = rp,
                .attachmentCount = att.size(),
                .pAttachments = att.data(),
                .width = m_width,
                .height = m_height,
                .layers = 1
            };
            vkCreateFramebuffer(dev, &fb_info, nullptr, &framebuffers[i]);
        }
    }
    void CreatePipeline() {
        auto vert = loadSPIRV(dev, "simple.vert.spv");
        auto frag = loadSPIRV(dev, "combined_image_sampler.frag.spv");

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
        ad[0].location = 0;
        ad[0].binding = 0;
        ad[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        ad[0].offset = 0;
        ad[1].location = 1;
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

        VkPipelineInputAssemblyStateCreateInfo ia_state = GfxPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        VkViewport vp = GfxPipelineViewport(m_width, m_height);
        VkRect2D scissor = GfxPipelineScissor(m_width, m_height);
        VkPipelineViewportStateCreateInfo vp_state = GfxPipelineViewportState(&vp, &scissor);
        VkPipelineRasterizationStateCreateInfo rs_state = GfxPipelineRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineMultisampleStateCreateInfo ms_state = GfxPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT);
        VkPipelineColorBlendAttachmentState blend_att_state = GfxPipelineColorBlendAttachmentState();
        VkPipelineColorBlendStateCreateInfo blend_state = GfxPipelineBlendState(&blend_att_state);

        std::array<VkGraphicsPipelineCreateInfo, 2> graphics_pipeline_info = {
            VkGraphicsPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stageCount = (uint32_t)shader_stage_info.size(),
                .pStages = shader_stage_info.data(),
                .pVertexInputState = &vert_input_state,
                .pInputAssemblyState = &ia_state,
                .pTessellationState = nullptr,
                .pViewportState = &vp_state,
                .pRasterizationState = &rs_state,
                .pMultisampleState = &ms_state,
                .pDepthStencilState = nullptr,
                .pColorBlendState = &blend_state,
                .pDynamicState = nullptr,
                .layout = pipeline_layout,
                .renderPass = rp,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = 0
            },
            VkGraphicsPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stageCount = (uint32_t)shader_stage_info.size(),
                .pStages = shader_stage_info.data(),
                .pVertexInputState = &vert_input_state,
                .pInputAssemblyState = &ia_state,
                .pTessellationState = nullptr,
                .pViewportState = &vp_state,
                .pRasterizationState = &rs_state,
                .pMultisampleState = &ms_state,
                .pDepthStencilState = nullptr,
                .pColorBlendState = &blend_state,
                .pDynamicState = nullptr,
                .layout = pipeline_layout,
                .renderPass = rp,
                .subpass = 1,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = 0
            },
        };
        vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, graphics_pipeline_info.size(), graphics_pipeline_info.data(), nullptr, pipeline.data());
        vkDestroyShaderModule(dev, vert, nullptr);
        vkDestroyShaderModule(dev, frag, nullptr);
    }
    void Gameloop() {
        uint32_t image_index;
        while (!glfwWindowShouldClose(window)) {
            // application / swapchain / presenter (display)
            // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
            vkAcquireNextImageKHR(dev, swapchain->get_swapchain(), UINT64_MAX, image_available_sema, VK_NULL_HANDLE, &image_index);

            VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo submit_info {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &image_available_sema, // can't submit until it has an available image
                .pWaitDstStageMask = &wait_dst_stage_mask,
                .commandBufferCount = 1,
                .pCommandBuffers = &cmdbuf[image_index],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &image_render_finished_sema, // GPU signals that it finishes rendering
            };
            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

            auto tmp_swaphcain = swapchain->get_swapchain();
            VkPresentInfoKHR present_info {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &image_render_finished_sema, // can't present until it finished rendering
                .swapchainCount = 1,
                .pSwapchains = &tmp_swaphcain,
                .pImageIndices = &image_index,
                .pResults = nullptr
            };
            vkQueuePresentKHR(queue, &present_info);
            glfwPollEvents();
        }
    }
    std::vector<VkCommandBuffer> cmdbuf;
    VkRenderPass rp;
    std::vector<VkFramebuffer> framebuffers;
    VkDescriptorPool ds_pool;
    std::array<VkDescriptorSet, 2> ds;
    VkDescriptorSetLayout dsl;
    VkPipelineLayout pipeline_layout;
    std::array<VkPipeline, 2> pipeline;
    GfxBuffer *vertex_buf;
    GfxImage2D *src_texture; // Subpass 0 sample src_texture to dst_texture
    GfxImage2D *dst_texture; // Subpass 1 sample dst_texture to swapchain
    VkSampler sampler;
};

}
int main() {
    common::TestSubpass T;
    T.Gameloop();
    return 0;
}
