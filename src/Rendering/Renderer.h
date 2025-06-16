#ifndef RENDERER_H
#define RENDERER_H
#include <functional>
#include <memory>

#include "Device.h"
#include "Swapchain.h"
#include "../Core/Window.h"


namespace womp {

    extern PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    extern PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

    class Renderer {
    public:
        explicit Renderer(Window& windowRef);
        ~Renderer();

        void initialise();

        void SetResizeCallback(const std::function<void(VkExtent2D)>& func) { m_resizeCallback = func; }
        [[nodiscard]] Swapchain& getSwapchain() const { return *m_swapChain; }

        [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const {
            assert(m_isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[m_currentFrameIndex];
        }

        [[nodiscard]] int getFrameIndex() const {
            assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
            return m_currentFrameIndex;
        }

        VkCommandBuffer BeginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer) const;
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer) const;
        [[nodiscard]] VkDevice getVkDevice() const { return m_device->GetVkDevice(); }
        [[nodiscard]] Device& getDevice() const { return *m_device; }

        [[nodiscard]] float GetAspectRatio() const { return m_swapChain->ExtentAspectRatio(); }
        [[nodiscard]] bool IsFrameInProgress() const { return m_isFrameStarted; }

        [[nodiscard]] int GetFrameIndex() const {
            assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
            return m_currentFrameIndex;
        }

        [[nodiscard]] Image& GetCurrentImage() const {
            assert(m_isFrameStarted && "Cannot get current image when frame not in progress");
            return m_swapChain->GetImage(static_cast<int>(m_currentImageIndex));
        }

        [[nodiscard]] Image& GetCurrentDepthImage() const {
            assert(m_isFrameStarted && "Cannot get current depth image when frame not in progress");
            return m_swapChain->GetDepthImage(static_cast<int>(m_currentImageIndex));
        }

        // void drawTexture(VkCommandBuffer commandBuffer, const Image& textureImage, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const {
        //     assert(m_isFrameStarted && "Cannot draw texture when frame not in progress");
        //     textureImage.TransitionImageLayout(commandBuffer, imageLayout);
        //     DebugLabel::BeginCmdLabel(commandBuffer, "DrawTexture");
        //     // Add your drawing commands here
        //     DebugLabel::EndCmdLabel(commandBuffer);
        // }

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        Window& m_window;

        std::unique_ptr<womp::Device> m_device;
        std::unique_ptr<Swapchain> m_swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t m_currentImageIndex{};
        int m_currentFrameIndex{0};
        bool m_isFrameStarted{false};

        std::function<void(VkExtent2D)> m_resizeCallback{};
    };
}


#endif //RENDERER_H
