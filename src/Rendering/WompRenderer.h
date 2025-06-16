#ifndef WOMPRENDERER_H
#define WOMPRENDERER_H
#include "Pipeline.h"
#include "Renderer.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "glm/vec4.hpp"

namespace womp {
    class WompRenderer {
    public:
        explicit WompRenderer(Device& deviceRef, Renderer& renderer);
        ~WompRenderer();

        void drawTexture(Image& image, float x, float y, float width, float height, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        void render(VkCommandBuffer commandBuffer, uint32_t imageIndex, const Swapchain& swapchain);

        [[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
        [[nodiscard]] const std::vector<VkDescriptorSet>& getDescriptorSets() const { return m_descriptorSets; }
    private:
        Renderer& m_renderer;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::vector<VkDescriptorSet> m_descriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;

        std::unique_ptr<Buffer
    };
}

#endif //WOMPRENDERER_H
