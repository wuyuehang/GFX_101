#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include "VulkanCommon.hpp"
#include "SkeletonVulkan.hpp"

#define USE_SEPARATE_IMAGE_SAMPLER 1

namespace common {

class TestMultiDescriptorSet : public SkeletonVulkan {
public:
    TestMultiDescriptorSet() : SkeletonVulkan(VK_API_VERSION_1_0, 800, 800) {
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
    ~TestMultiDescriptorSet() {
        vkDeviceWaitIdle(dev);
        delete vertex_buf; delete texture;
        vkDestroySampler(dev, sampler, nullptr);
        vkDestroySemaphore(dev, image_available_sema, nullptr);
        vkDestroySemaphore(dev, image_render_finished_sema, nullptr);
        vkDestroyPipeline(dev, pipeline, nullptr);
        vkDestroyPipelineLayout(dev, pipeline_layout, nullptr);
        for (auto & it : dsl) {
            vkDestroyDescriptorSetLayout(dev, it, nullptr);
        }
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

        texture = new GfxImage2D(this);
        texture->bake("../resource/vegeta.png", VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        texture->transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        texture->set_descriptor_info(VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    void CreateDescriptor() {
        // binding = 0, set = 0
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings = {
                GfxDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            };
            VkDescriptorSetLayoutCreateInfo dsl_info = GfxDescriptorSetLayoutCreateInfo(bindings);
            vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl[0]);
        }
        // binding = 0, set = 1
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings = {
                GfxDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            };
            VkDescriptorSetLayoutCreateInfo dsl_info = GfxDescriptorSetLayoutCreateInfo(bindings);
            vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &dsl[1]);
        }
        // shared pipeline layout
        VkPipelineLayoutCreateInfo pipeline_layout_info = GfxPipelineLayoutCreateInfo(dsl.data(), dsl.size());
        vkCreatePipelineLayout(dev, &pipeline_layout_info, nullptr, &pipeline_layout);

        // shared descriptor pool
        std::vector<VkDescriptorPoolSize> pool_size = {
            GfxDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1),
            GfxDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1)
        };
        VkDescriptorPoolCreateInfo pool_info = GfxDescriptorPoolCreateInfo(ds.size(), pool_size);
        vkCreateDescriptorPool(dev, &pool_info, nullptr, &ds_pool);

        // descriptor
        VkDescriptorImageInfo smp_info {
            .sampler = sampler,
            .imageView = VK_NULL_HANDLE,
            .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkDescriptorSetAllocateInfo alloc_info = GfxDescriptorSetAllocateInfo(ds_pool, dsl.data(), dsl.size());
        vkAllocateDescriptorSets(dev, &alloc_info, ds.data());
        std::vector<VkWriteDescriptorSet> w = {
            GfxWriteDescriptorSet(ds[0], 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &texture->get_descriptor_info()),
            GfxWriteDescriptorSet(ds[1], 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &smp_info),
        };
        vkUpdateDescriptorSets(dev, w.size(), w.data(), 0, nullptr);
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
            VkClearValue clear_value;
            clear_value.color = VkClearColorValue {};
            VkRenderPassBeginInfo begin_renderpass_info {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = rp,
                .framebuffer = framebuffers[i],
                .renderArea = VkRect2D {
                    .offset = VkOffset2D { 0, 0 },
                    .extent = VkExtent2D { 800, 800 },
                },
                .clearValueCount = 1,
                .pClearValues = &clear_value,
            };
            vkCmdBeginRenderPass(cmdbuf[i], &begin_renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkDeviceSize offsets[] { 0 };
            vkCmdBindVertexBuffers(cmdbuf[i], 0, 1, &vertex_buf->get_buffer(), offsets);
            vkCmdBindDescriptorSets(cmdbuf[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, ds.size(), ds.data(), 0, nullptr);
            vkCmdDraw(cmdbuf[i], 6, 1, 0, 0);
            vkCmdEndRenderPass(cmdbuf[i]);
            end_cmdbuf(cmdbuf[i]);
        }
    }
    void CreateRenderPass() {
        VkAttachmentDescription att_desc {
            .flags = 0,
            .format = VK_FORMAT_B8G8R8A8_SRGB, // same as swapchain
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // image layout changes within a render pass witout image memory barrier
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference att_ref {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
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
        framebuffers.resize(swapchain->get_swapchain_images().size());
        auto swapchain_views = swapchain->get_swapchain_views();
        for (uint32_t i = 0; i < framebuffers.size(); i++) {
            VkFramebufferCreateInfo fb_info {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = rp,
                .attachmentCount = 1,
                .pAttachments = &swapchain_views[i],
                .width = 800,
                .height = 800,
                .layers = 1
            };
            vkCreateFramebuffer(dev, &fb_info, nullptr, &framebuffers[i]);
        }
    }
    void CreatePipeline() {
        auto vert = loadSPIRV(dev, "simple.vert.spv");
        auto frag = loadSPIRV(dev, "multi_descriptor_set.frag.spv");

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

        VkGraphicsPipelineCreateInfo graphics_pipeline_info {
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
        };
        vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &pipeline);
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
    std::array<VkDescriptorSetLayout, 2> dsl;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    GfxBuffer *vertex_buf;
    GfxImage2D *texture;
    VkSampler sampler;
};

}
int main() {
    common::TestMultiDescriptorSet T;
    T.Gameloop();
    return 0;
}
