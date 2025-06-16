#include "WompRenderer.h"

#include "DebugLabel.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"

womp::WompRenderer::WompRenderer(Device& deviceRef, Renderer& renderer): m_renderer(renderer), m_framesInFlight(Swapchain::MAX_FRAMES_IN_FLIGHT) {

    m_descriptorPool = DescriptorPool::Builder(deviceRef)
            .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT * 2)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, Swapchain::MAX_FRAMES_IN_FLIGHT * 3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Swapchain::MAX_FRAMES_IN_FLIGHT * 3)
            .build();


    m_descriptorSetLayout = DescriptorSetLayout::Builder(deviceRef)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //exposure ubo
            .build();

  const std::array descriptorSetLayouts = {
        m_descriptorSetLayout->getDescriptorSetLayout()
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::vec4);
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    if (vkCreatePipelineLayout(deviceRef.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not make pipleine layout");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    VkFormat swapchainFormat = renderer.getSwapchain().GetSwapChainImageFormat();

    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.colorAttachments = {swapchainFormat};

    m_pipeline = std::make_unique<Pipeline>(
        deviceRef,
        "shaders/basic.vert.spv",
        "shaders/basic.frag.spv",
        pipelineConfig
    );

    m_descriptorSets.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    auto dummyImage = Image(deviceRef, {100, 100}, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    auto dummyInfo = dummyImage.descriptorInfo();

    for (size_t i{0}; i < m_framesInFlight; i++) {
        m_descriptorPool->allocateDescriptor(m_descriptorSetLayout->getDescriptorSetLayout(), m_descriptorSets[i]);

        DescriptorWriter(*m_descriptorSetLayout, *m_descriptorPool)
                .writeImage(0, &dummyInfo)
                .build(m_descriptorSets[i]);
    }

}

womp::WompRenderer::~WompRenderer() {
    vkDestroyPipelineLayout(m_renderer.getDevice().GetVkDevice(), m_pipelineLayout, nullptr);
}

void womp::WompRenderer::drawTexture(Image& image, float x, float y, float width, float height, const glm::vec4& color) {

}

void womp::WompRenderer::render(VkCommandBuffer commandBuffer, uint32_t imageIndex, const Swapchain& swapchain) {

    DebugLabel::BeginCmdLabel(commandBuffer, "Blitting pass", glm::vec4(1.f, 0.7f, 0.1f, 1));

    Image& currentImage = swapchain.GetImage(static_cast<int>(imageIndex));

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(currentImage.GetExtent().width);
    viewport.height = static_cast<float>(currentImage.GetExtent().height);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = currentImage.GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex], 0, nullptr);

    m_pipeline->bind(commandBuffer);
    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);


}
