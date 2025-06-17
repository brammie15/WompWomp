#ifndef WOMPRENDERER_H
#define WOMPRENDERER_H

#include "Resources/Buffer.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "glm/vec4.hpp"

namespace womp {

    using TextureHandle = uint32_t;

    struct DrawCommand {
        TextureHandle texture;
        glm::vec4 dstRect; // x, y, w, h
        glm::vec4 srcRect; // x, y, w, h
        float rotation = 0.0f; // in radians
        glm::vec4 color;   // optional
    };


    struct Texture {
        std::unique_ptr<Image> image;         // Vulkan image abstraction
        VkDescriptorSet descriptorSet{};      // Descriptor set for sampling
        glm::ivec2 size{};                    // Width, height in pixels (optional)
    };

    struct PushConstants {
        glm::vec4 dstRect; // x, y, width, height
        float rotation;    // rotation in radians
    };

    class WompRenderer {
    public:
        explicit WompRenderer(Window& windowRef);
        ~WompRenderer();

        void drawTexture(TextureHandle image, float x, float y, float width, float height, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        void drawTexture(TextureHandle image, glm::vec2 position, glm::vec2 size, float rotation = 0.0f, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        void render();

        [[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
        [[nodiscard]] const std::vector<VkDescriptorSet>& getDescriptorSets() const { return m_descriptorSets; }

        TextureHandle createTexture(const std::string& filepath);

        void waitIdle() const;
    private:
        Device* m_device;
        std::unique_ptr<Renderer> m_renderer;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::vector<VkDescriptorSet> m_descriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;

        std::unique_ptr<Buffer> m_vertexBuffer{};
        std::unique_ptr<Buffer> m_indexBuffer{};

        std::unique_ptr<Image> m_dummyImage{};
        std::unique_ptr<Image> m_testImage{};

        std::vector<DrawCommand> m_pendingDrawCommands;

        std::unordered_map<TextureHandle, Texture> m_textures;
        TextureHandle m_nextHandle = 1;
    };
}

#endif //WOMPRENDERER_H
