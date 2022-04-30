#include <cstring>
#include "HelloVulkan.hpp"

HelloVulkan::HelloVulkan() : vulkan_swapchain(this), m_ctrl(new TrackballController(this)),
    m_current_frame(0) {
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

    bake_imgui();

    CreateResource();
    {
        bake_axis_DescriptorSetLayout(axis_pipe);
        bake_axis_DescriptorSet(axis_pipe);
        bake_axis_Pipeline(axis_pipe);
    }
    {
        bake_default_DescriptorSetLayout(default_pipe);
        bake_default_DescriptorSet(default_pipe);
        bake_default_Pipeline(default_pipe);
    }
    {
        bake_wireframe_DescriptorSetLayout(wireframe_pipe);
        bake_wireframe_DescriptorSet(wireframe_pipe);
        bake_wireframe_Pipeline(wireframe_pipe);
    }
    {
        bake_visualize_vertex_normal_DescriptorSetLayout(visualize_vertex_normal_pipe);
        bake_visualize_vertex_normal_DescriptorSet(visualize_vertex_normal_pipe);
        bake_visualize_vertex_normal_Pipeline(visualize_vertex_normal_pipe);
    }
    {
        bake_phong_DescriptorSetLayout(phong_pipe);
        bake_phong_DescriptorSet(phong_pipe);
        bake_phong_Pipeline(phong_pipe);
    }
}

HelloVulkan::~HelloVulkan() {
    vkDeviceWaitIdle(dev);
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(dev, imgui_pool, nullptr);
    }
    clean_VulkanPipe(phong_pipe);
    clean_VulkanPipe(visualize_vertex_normal_pipe);
    clean_VulkanPipe(wireframe_pipe);
    clean_VulkanPipe(default_pipe);
    clean_VulkanPipe(axis_pipe);

    // SCENE
    for (const auto& it : scene_uniform) {
        vkFreeMemory(dev, it->get_memory(), nullptr);
        vkDestroyBuffer(dev, it->get_buffer(), nullptr);
    }
    // DEFAULT VERTEX
    {
        for (const auto& it : default_mvp_uniform) {
            vkFreeMemory(dev, it->get_memory(), nullptr);
            vkDestroyBuffer(dev, it->get_buffer(), nullptr);
        }
    }
    delete default_vertex;
    // AXIS
    {
        delete axis_vertex;
        for (const auto& it : axis_mvp_uniform) {
            vkFreeMemory(dev, it->get_memory(), nullptr);
            vkDestroyBuffer(dev, it->get_buffer(), nullptr);
        }
    }

    for (auto& it : framebuffers) {
        vkDestroyFramebuffer(dev, it, nullptr);
    }
    vkDestroyRenderPass(dev, rp, nullptr);
    delete depth;
    vkDestroySampler(dev, default_sampler, nullptr);
    delete default_tex;
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
    delete m_ctrl;
}

void HelloVulkan::CreateResource() {
    // AXIS
    {
        glm::mat4 pre_rotation_mat = glm::rotate(glm::scale(glm::mat4(1.0), glm::vec3(1.25)), (float)glm::radians(180.0), glm::vec3(1.0, 0.0, 0.0));
        axis_mesh.load("./assets/obj/axis.obj", pre_rotation_mat);
        axis_vertex = new BufferObj(this);
        axis_vertex->init(axis_mesh.get_vertices().size() * sizeof(Mesh::Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        axis_vertex->upload(axis_mesh.get_vertices().data(), axis_mesh.get_vertices().size() * sizeof(Mesh::Vertex));

        axis_mvp_uniform.resize(m_max_inflight_frames);
        for (auto i = 0; i < axis_mvp_uniform.size(); i++) {
            axis_mvp_uniform[i] = new BufferObj(this);
            axis_mvp_uniform[i]->init(sizeof(struct MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
    }
    // DEFAULT OBJECT
    {
        default_mesh.load("./assets/obj/Buddha.obj", glm::mat4(1.0));
        default_vertex = new BufferObj(this);
        default_vertex->init(default_mesh.get_vertices().size() * sizeof(Mesh::Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        default_vertex->upload(default_mesh.get_vertices().data(), default_mesh.get_vertices().size() * sizeof(Mesh::Vertex));

        default_mvp_uniform.resize(m_max_inflight_frames);
        for (auto i = 0; i < default_mvp_uniform.size(); i++) {
            default_mvp_uniform[i] = new BufferObj(this);
            default_mvp_uniform[i]->init(sizeof(struct MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        default_tex = new ImageObj(this);
        default_tex->bake("./assets/obj/Buddha.jpg");
        default_tex->transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    // SCENE
    scene_uniform.resize(m_max_inflight_frames);
    for (auto i = 0; i < scene_uniform.size(); i++) {
        scene_uniform[i] = new BufferObj(this);
        scene_uniform[i]->init(sizeof(SCENE), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}

void HelloVulkan::BakeCommand(uint32_t frame_nr) {
    run_if_default(default_pipe, frame_nr);
    run_if_wireframe(wireframe_pipe, frame_nr);
    run_if_vnn(visualize_vertex_normal_pipe, frame_nr);
    run_if_phong(phong_pipe, frame_nr);
}

void HelloVulkan::Gameloop() {
    // initialize ImGui tuning values
    {
        m_display_axis = false;
        m_exclusive_mode = DEFAULT_MODE;
        m_roughness = 1.0;
    }
    while (!glfwWindowShouldClose(window)) {
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Describe the ImGui UI
            ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always); // Pin the UI
            ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
            ImGui::Begin("Hello, Vulkan!");
            ImGui::Checkbox("Axis", &m_display_axis);
            ImGui::RadioButton("Default", &m_exclusive_mode, DEFAULT_MODE);
            ImGui::RadioButton("Wireframe", &m_exclusive_mode, WIREFRAME_MODE);
            ImGui::RadioButton("Vertex Normal", &m_exclusive_mode, VISUALIZE_VERTEX_NORMAL_MODE);
            ImGui::RadioButton("Phong", &m_exclusive_mode, PHONG_MODE);
            ImGui::SliderFloat("Roughness", &m_roughness, 0.01, 256.0);
            ImGui::Text("Eye @ (%.1f, %.1f, %.1f)", m_ctrl->get_eye().x, m_ctrl->get_eye().y, m_ctrl->get_eye().z);
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
            ImGui::Render();
        }

        vkWaitForFences(dev, 1, &fence[m_current_frame], VK_TRUE, UINT64_MAX);

        // application / swapchain / presenter (display)
        // swapchain, a pool of availabe images, a pool of to be presented images, a presented image
        uint32_t image_index;
        vkAcquireNextImageKHR(dev, vulkan_swapchain.swapchain, UINT64_MAX, image_available[m_current_frame], VK_NULL_HANDLE, &image_index);

        m_ctrl->handle_input();
        {
            MVP ubo {};
            ubo.model = m_ctrl->get_model() * default_mesh.get_model_mat();
            ubo.view = m_ctrl->get_view();
            ubo.proj = m_ctrl->get_proj();

            void *buf_ptr;
            vkMapMemory(dev, default_mvp_uniform[image_index]->get_memory(), 0, sizeof(ubo), 0, &buf_ptr);
            memcpy(buf_ptr, &ubo, sizeof(ubo));
            vkUnmapMemory(dev, default_mvp_uniform[image_index]->get_memory());
        }
        {
            // axis
            MVP ubo {};
            ubo.model = m_ctrl->get_model() * axis_mesh.get_model_mat();
            ubo.view = m_ctrl->get_view();
            ubo.proj = m_ctrl->get_proj();

            void *buf_ptr;
            vkMapMemory(dev, axis_mvp_uniform[image_index]->get_memory(), 0, sizeof(ubo), 0, &buf_ptr);
            memcpy(buf_ptr, &ubo, sizeof(ubo));
            vkUnmapMemory(dev, axis_mvp_uniform[image_index]->get_memory());
        }
        {
            // handle light position
            SCENE lt {};
            lt.light_loc = glm::vec3(m_ctrl->get_view() * glm::vec4(0.0, 0.0, 3.0, 1.0));
            lt.roughness = m_roughness;

            void *buf_ptr;
            vkMapMemory(dev, scene_uniform[image_index]->get_memory(), 0, sizeof(lt), 0, &buf_ptr);
            memcpy(buf_ptr, &lt, sizeof(lt));
            vkUnmapMemory(dev, scene_uniform[image_index]->get_memory());
        }
        BakeCommand(m_current_frame);

        vkResetFences(dev, 1, &fence[m_current_frame]);

        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_available[m_current_frame]; // application can't submit until it has an available image
        VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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