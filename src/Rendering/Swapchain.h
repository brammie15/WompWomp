#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <memory>
#include <vector>
#include <VkBootstrap.h>
#include "Device.h"
#include "Resources/Image.h"

namespace womp {
    class Swapchain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        explicit Swapchain(Device& deviceRef, VkExtent2D windowExtent, Swapchain* previous = nullptr);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        [[nodiscard]] VkExtent2D GetSwapChainExtent() const { return m_swapchain.extent; }
        [[nodiscard]] size_t imageCount() const { return m_swapchain.image_count; }
        [[nodiscard]] VkFormat GetSwapChainImageFormat() const { return m_swapchain.image_format; }
        [[nodiscard]] uint32_t GetWidth() const { return m_swapchain.extent.width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_swapchain.extent.height; }
        [[nodiscard]] float ExtentAspectRatio() const;

        [[nodiscard]] VkImageView GetImageView(int index) const {
            return m_swapChainImages[index]->GetImageView();
        }

        VkResult acquireNextImage(uint32_t* imageIndex);
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, const uint32_t* imageIndex);

        [[nodiscard]] Image& GetImage(int index) const;
        [[nodiscard]] Image& GetDepthImage(int index) const;

        const vkb::Swapchain& GetVkbSwapchain() const { return m_swapchain; }

    private:
        void createDepthResources();
        void createSyncObjects();

        void createDepthImages();
        [[nodiscard]] VkFormat findDepthFormat() const;

        Device& m_device;
        vkb::Swapchain m_swapchain;

        VkExtent2D m_windowExtent{};
        VkExtent2D m_swapchainExtent{};

        VkFormat m_swapChainImageFormat;
        VkFormat m_swapChainDepthFormat;

        std::vector<std::unique_ptr<Image>> m_swapChainImages{};
        std::vector<std::unique_ptr<Image>> m_depthImages{};

        std::vector<VkSemaphore> m_imageAvailableSemaphores{};
        std::vector<VkSemaphore> m_renderFinishedSemaphores{};
        std::vector<VkFence> m_inFlightFences{};
        std::vector<VkFence> m_imagesInFlight{};
        size_t m_currentFrame{ 0 };
    };
}

#endif // SWAPCHAIN_H