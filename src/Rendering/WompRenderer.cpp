#include "WompRenderer.h"

#include "DebugLabel.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"


womp::WompRenderer::WompRenderer(Window& windowRef): m_framesInFlight(Swapchain::MAX_FRAMES_IN_FLIGHT) {
    m_renderer = std::make_unique<Renderer>(windowRef);

    Device& deviceRef = m_renderer->getDevice();
    m_device = &deviceRef;

    m_pendingDrawCommands.resize(m_framesInFlight * 10);

    m_descriptorPool = DescriptorPool::Builder(deviceRef)
            .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT * 100)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, Swapchain::MAX_FRAMES_IN_FLIGHT * 100)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swapchain::MAX_FRAMES_IN_FLIGHT * 2)
            .build();

    m_screenSizeDescriptorSetLayout = DescriptorSetLayout::Builder(deviceRef)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    m_textureDescriptorSetLayout = DescriptorSetLayout::Builder(deviceRef)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    const std::array descriptorSetLayouts = {
        m_screenSizeDescriptorSetLayout->getDescriptorSetLayout(),
        m_textureDescriptorSetLayout->getDescriptorSetLayout()
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    if (vkCreatePipelineLayout(deviceRef.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not make pipleine layout");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    VkFormat swapchainFormat = m_renderer->getSwapchain().GetSwapChainImageFormat();

    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.colorAttachments = {swapchainFormat};

    pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

    m_pipeline = std::make_unique<Pipeline>(
        deviceRef,
        "shaders/basic.vert.spv",
        "shaders/basic.frag.spv",
        pipelineConfig
    );

    m_textureDescriptorSets.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
    m_screenSizeDescriptorSets.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    m_dummyImage = std::make_unique<Image>(deviceRef, VkExtent2D{100, 100}, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    deviceRef.TransitionImageLayout(
        m_dummyImage->getImage(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1
    );

    const auto dummyInfo = m_dummyImage->descriptorInfo();

    m_screenSizeUniformBuffers.resize(m_framesInFlight);


    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_textureDescriptorSetLayout->getDescriptorSetLayout(), m_textureDescriptorSets[i]);

        DescriptorWriter(*m_textureDescriptorSetLayout, *m_descriptorPool)
                .writeImage(0, &dummyInfo)
                .build(m_textureDescriptorSets[i]);

        m_descriptorPool->allocateDescriptor(m_screenSizeDescriptorSetLayout->getDescriptorSetLayout(), m_screenSizeDescriptorSets[i]);

        m_screenSizeUniformBuffers[i] = std::make_unique<Buffer>(
            deviceRef,
            sizeof(glm::vec2),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            true
        );

        auto screenSizeInfo = m_screenSizeUniformBuffers[i]->descriptorInfo();
        DescriptorWriter(*m_screenSizeDescriptorSetLayout, *m_descriptorPool)
            .writeBuffer(0, &screenSizeInfo)
            .build(m_screenSizeDescriptorSets[i]);
    }

    const std::vector<Vertex> verticies = {
        Vertex{glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2,
        2, 3, 0
    };

    const auto vertexStaging = std::make_unique<Buffer>(deviceRef, sizeof(Vertex) * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, true);

    vertexStaging->map();
    vertexStaging->copyTo(verticies.data(), sizeof(Vertex) * verticies.size());
    vertexStaging->unmap();

    m_vertexBuffer = std::make_unique<Buffer>(
        deviceRef,
        sizeof(Vertex) * 4,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO,
        true
    );

    vertexStaging->copyToBuffer(m_vertexBuffer.get(), sizeof(Vertex) * 4);

    const auto indexStaging = std::make_unique<Buffer>(
        deviceRef,
        sizeof(uint32_t) * 6, // 6 indices for two triangles
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        true
    );

    indexStaging->map();
    indexStaging->copyTo(indices.data(), sizeof(uint32_t) * indices.size());
    indexStaging->unmap();

    m_indexBuffer = std::make_unique<Buffer>(
        deviceRef,
        sizeof(uint32_t) * 6, // 6 indices for two triangles
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO,
        true
    );

    indexStaging->copyToBuffer(m_indexBuffer.get(), sizeof(uint32_t) * 6);
}

womp::WompRenderer::~WompRenderer() {
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_dummyImage.reset();
    m_pipeline.reset();
    m_descriptorPool.reset();
    m_textureDescriptorSetLayout.reset();
    m_screenSizeDescriptorSetLayout.reset();
    m_screenSizeUniformBuffers.clear();
    for (auto& [handle, texture]: m_textures) {
        texture.image.reset();
    }
    m_pendingDrawCommands.clear();

    vkDestroyPipelineLayout(m_renderer->getDevice().GetVkDevice(), m_pipelineLayout, nullptr);
    this->waitIdle();
    m_renderer.reset();
}

void womp::WompRenderer::drawTexture(TextureHandle image, WP_Rect srcRect, WP_Rect dstRect, glm::vec4 color) {;
    m_pendingDrawCommands.push_back(DrawCommand{
        .texture = image,
        .srcRect = srcRect,
        .dstRect = dstRect,
        .color = color
    });
}

void womp::WompRenderer::drawTexture(TextureHandle image, WP_Rect srcRect, WP_Rect dstRect, float rotation, glm::vec4 color) {
    m_pendingDrawCommands.push_back(DrawCommand{
        .texture = image,
        .srcRect = srcRect,
        .dstRect = dstRect,
        .rotation = rotation,
        .color = color
    });
}

void womp::WompRenderer::drawTexture(TextureHandle image, glm::vec2 position, glm::vec2 size, glm::vec4 color) {
    glm::vec2 textureSize = m_textures[image].size;
    const WP_Rect srcRect{0, 0, textureSize.x, textureSize.y};
    const WP_Rect dstRect{position.x, position.y, size.x, size.y};
    drawTexture(image, srcRect, dstRect, color);
}

void womp::WompRenderer::render() {
    if (const VkCommandBuffer commandBuffer = m_renderer->BeginFrame()) {
        const int frameIndex = m_renderer->getFrameIndex();

        auto& screenSizeBuffer = m_screenSizeUniformBuffers[frameIndex];
        glm::vec2 screenSize{ m_renderer->getSwapchain().GetWidth(), m_renderer->getSwapchain().GetHeight() };
        screenSizeBuffer->copyTo(&screenSize, sizeof(screenSize));
        screenSizeBuffer->flush();


        m_renderer->beginSwapChainRenderPass(commandBuffer);
        DebugLabel::BeginCmdLabel(commandBuffer, "Draw Textures", glm::vec4(0.1f, 0.8f, 0.2f, 1));

        m_pipeline->bind(commandBuffer);
        const VkBuffer vertexBuffers[] = {m_vertexBuffer->getBuffer()};
        constexpr VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);


        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &m_screenSizeDescriptorSets[frameIndex],
            0,
            nullptr
        );

        // Draw all pending draw commands
        for (const auto& cmd: m_pendingDrawCommands) {
            // if (cmd.texture == static_cast<uint32_t>(-1)) {
            //     // Draw a rectangle with color
            //     PushConstants push = {
            //         .dstRect = cmd.dstRect,
            //         .rotation = 0.0f,
            //         .srcRect = glm::vec4(0, 0, 0, 0), // No texture source rect
            //     };
            //
            //     vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push);
            //
            //
            //     continue;
            // }


            auto it = m_textures.find(cmd.texture);
            if (it == m_textures.end()) continue;

            const Texture& tex = it->second;

            glm::vec4 srcUV = cmd.srcRect.toVec4();
            if (srcUV.z > 0 && srcUV.w > 0) {
                srcUV.x /= tex.size.x;
                srcUV.y /= tex.size.y;
                srcUV.z /= tex.size.x;
                srcUV.w /= tex.size.y;
            } else {
                srcUV = glm::vec4(0, 0, 1, 1);
            }

            PushConstants push = {
                .srcRect = srcUV,
                .dstRect = cmd.dstRect.toVec4(),
                .rotation = cmd.rotation,
                .color = cmd.color
            };

            vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &push);


            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &tex.descriptorSet, 0, nullptr);


            vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
        }

        DebugLabel::EndCmdLabel(commandBuffer);
        m_renderer->endSwapChainRenderPass(commandBuffer);
        m_renderer->endFrame();

        // Clear draw queue AFTER render is finished
        m_pendingDrawCommands.clear();
    }
}


womp::TextureHandle womp::WompRenderer::createTexture(const std::string& filepath) {
    Device& device = m_renderer->getDevice();

    auto image = std::make_unique<Image>(
        device,
        filepath,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_FILTER_LINEAR
    );

    device.TransitionImageLayout(
        image->getImage(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1
    );

    VkDescriptorSet set;

    m_descriptorPool->allocateDescriptor(m_textureDescriptorSetLayout->getDescriptorSetLayout(), set);

    const auto imageInfo = image->descriptorInfo();

    DescriptorWriter(*m_textureDescriptorSetLayout, *m_descriptorPool)
            .writeImage(0, &imageInfo)
            .build(set);

    const auto imageSize = glm::vec2(image->GetExtent().width, image->GetExtent().height);

    Texture tex{
        .image = std::move(image),
        .descriptorSet = set,
        .size = imageSize,
    };

    TextureHandle handle = m_nextHandle++;
    m_textures.emplace(handle, std::move(tex));
    return handle;
}

void womp::WompRenderer::waitIdle() const {
    vkDeviceWaitIdle(m_renderer->getDevice().GetVkDevice());
}
