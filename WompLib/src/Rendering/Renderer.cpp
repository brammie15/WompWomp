#include <Womp/Renderer.h>

#include "DebugLabel.h"

womp::Renderer::Renderer(womp::Window& windowRef): m_window{windowRef} {
    m_device = std::make_unique<Device>(windowRef);
    initialise();
    createCommandBuffers();
}

womp::Renderer::~Renderer() {
    freeCommandBuffers();
    m_swapChain.reset();
    m_device.reset();
}

void womp::Renderer::initialise() {
    const auto extent = VkExtent2D(m_window.getWidth(), m_window.getHeight());
    m_swapChain = std::make_unique<Swapchain>(*m_device, extent);
}

VkCommandBuffer womp::Renderer::BeginFrame() {
    assert(!m_isFrameStarted && "Frame not in progress yet??");

    const auto result = m_swapChain->acquireNextImage(&m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_isFrameStarted = true;

    const auto commandBuffer = GetCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }


    return commandBuffer;
}

void womp::Renderer::endFrame() {
    assert(m_isFrameStarted && "Can't call endFrame while frame is not in progress");


    const auto commandBuffer = GetCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    const auto result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasResized()) {
        m_window.resetResizeFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_isFrameStarted = false;
    m_currentFrameIndex = (m_currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void womp::Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) const {
    assert(m_isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == GetCurrentCommandBuffer() &&
        "Can't begin render pass on command buffer from a different frame");

    m_swapChain->GetImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
        commandBuffer,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    );

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    const VkRenderingAttachmentInfoKHR color_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_swapChain->GetImageView(static_cast<int>(m_currentImageIndex)),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearValues[0],
    };

    const VkRenderingAttachmentInfo depth_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).GetImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        // .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearValues[1],
    };

    const VkRenderingInfoKHR render_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {
            .offset = {0, 0},
            .extent = m_swapChain->GetSwapChainExtent(),
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info,
    };

    DebugLabel::BeginCmdLabel(
        commandBuffer,
        "RenderPass",
        {1.f, 1.0f, 0.0f, 1.0f}
    );

    vkCmdBeginRendering(commandBuffer, &render_info);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
    viewport.height = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    const VkRect2D scissor{{0, 0}, m_swapChain->GetSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void womp::Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) const {
    assert(m_isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == GetCurrentCommandBuffer() &&
        "Can't end render pass on command buffer from a different frame");

    vkCmdEndRendering(commandBuffer);

    m_swapChain->GetImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
        commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    );

    m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex)).TransitionImageLayout(
        commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );

    DebugLabel::EndCmdLabel(commandBuffer);
}

void womp::Renderer::createCommandBuffers() {
    commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device->getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device->GetVkDevice(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (int index = 0; index < commandBuffers.size(); ++index) {
        DebugLabel::NameCommandBuffer(commandBuffers[index], "CommandBuffer: " + std::to_string(index));
    }
}

void womp::Renderer::freeCommandBuffers() {
    vkFreeCommandBuffers(
           m_device->GetVkDevice(),
           m_device->getCommandPool(),
           static_cast<uint32_t>(commandBuffers.size()),
           commandBuffers.data()
       );
    commandBuffers.clear();
}

void womp::Renderer::recreateSwapChain() {
    int width = m_window.getWidth();
    int height = m_window.getHeight();
    while (width == 0 || height == 0) {
        width = m_window.getWidth();
        height = m_window.getHeight();
        glfwWaitEvents(); // or equivalent
    }

    vkDeviceWaitIdle(m_device->GetVkDevice());

    // freeCommandBuffers();  // If required
    m_swapChain = std::make_unique<Swapchain>(*m_device, VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, m_swapChain.get());
    // createCommandBuffers(); // If required
}
