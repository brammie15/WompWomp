#include "Swapchain.h"
#include <array>
#include <stdexcept>
#include <limits>

namespace womp {
    Swapchain::Swapchain(Device& deviceRef, VkExtent2D windowExtent, Swapchain* previous)
        : m_device(deviceRef), m_windowExtent{windowExtent}, m_swapchainExtent{windowExtent} {

        vkb::SwapchainBuilder swapchainBuilder{m_device.GetVkbDevice()};

        auto builder = swapchainBuilder
            .set_desired_extent(windowExtent.width, windowExtent.height)
            .set_desired_format(VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        if (previous && previous->m_swapchain != VK_NULL_HANDLE) {
            builder.set_old_swapchain(previous->m_swapchain);
        }

        auto swapchain_ret = builder.build();
        if (!swapchain_ret) {
            throw std::runtime_error("Failed to create swapchain: " + swapchain_ret.error().message());
        }
        m_swapchain = swapchain_ret.value();

        // Create wrapper images for the swapchain images
        for (auto& image : m_swapchain.get_images().value()) {
            auto vkImage = std::make_unique<Image>(
                m_device,
                m_swapchain.extent,
                m_swapchain.image_format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY,
                image);
            m_swapChainImages.push_back(std::move(vkImage));
        }

        createDepthResources();
        createSyncObjects();
    }

    Swapchain::~Swapchain() {
        m_swapChainImages.clear();
        m_depthImages.clear();

        // Cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device.GetVkDevice(), m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device.GetVkDevice(), m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device.GetVkDevice(), m_inFlightFences[i], nullptr);
        }

        vkb::destroy_swapchain(m_swapchain);
    }

    float Swapchain::ExtentAspectRatio() const {
        return static_cast<float>(m_swapchain.extent.width) / static_cast<float>(m_swapchain.extent.height);
    }

    Image& Swapchain::GetImage(int index) const {
        assert(index < m_swapChainImages.size() && "Image index out of range");
        return *m_swapChainImages[index];
    }

    Image& Swapchain::GetDepthImage(int index) const {
        assert(index < m_depthImages.size() && "Depth image index out of range");
        return *m_depthImages[index];
    }

    VkResult Swapchain::acquireNextImage(uint32_t* imageIndex) {
        vkWaitForFences(
            m_device.GetVkDevice(),
            1,
            &m_inFlightFences[m_currentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

        const VkResult result = vkAcquireNextImageKHR(
            m_device.GetVkDevice(),
            m_swapchain,
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailableSemaphores[m_currentFrame],
            VK_NULL_HANDLE,
            imageIndex);

        return result;
    }

    VkResult Swapchain::submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex) {
        if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(m_device.GetVkDevice(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;

        VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        const VkQueue graphicsQueue = m_device.GetVkbDevice().get_queue(vkb::QueueType::graphics).value();


        vkResetFences(m_device.GetVkDevice(), 1, &m_inFlightFences[m_currentFrame]);
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        const VkSwapchainKHR swapChains[] = {m_swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;


        const VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }


    void Swapchain::createDepthResources() {
        m_depthImages.clear();
        m_swapChainDepthFormat = findDepthFormat();

        for (size_t i = 0; i < imageCount(); i++) {
            auto depthImage = std::make_unique<Image>(
                m_device,
                m_swapchainExtent,
                m_swapChainDepthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
            m_depthImages.push_back(std::move(depthImage));
        }
    }

    void Swapchain::createSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_device.GetVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device.GetVkDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device.GetVkDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    VkFormat Swapchain::findDepthFormat() const {
        return m_device.FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
}