#ifndef WOMPRENDERER_H
#define WOMPRENDERER_H

#include "Resources/Buffer.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Core/WompMath.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "glm/vec4.hpp"

namespace womp {

    //Uniform setup
    //Set 0, binding 0; screensize vec2
    //set 1, binding 0; texture sampler


    using TextureHandle = uint32_t;

    struct DrawCommand {
        TextureHandle texture;
        WP_Rect srcRect;
        WP_Rect dstRect;
        float rotation = 0.0f;
        glm::vec4 color = glm::vec4(1.0f);
    };


    struct Texture {
        std::unique_ptr<Image> image;         // Vulkan image abstraction
        VkDescriptorSet descriptorSet{};      // Descriptor set for sampling
        glm::ivec2 size{};                    // Width, height in pixels (optional)
    };

    struct alignas(16) PushConstants {
        glm::vec4 srcRect;  // x, y, width, height
        glm::vec4 dstRect;
        float rotation;
        float _pad[3];
        glm::vec4 color;
    };

    class WompRenderer {
    public:
        explicit WompRenderer(Window& windowRef);
        ~WompRenderer();

        void drawTexture(TextureHandle image, WP_Rect srcRect, WP_Rect dstRect, glm::vec4 color = glm::vec4(1.0f));
        void drawTexture(TextureHandle image, WP_Rect srcRect, WP_Rect dstRect, float rotation, glm::vec4 color = glm::vec4(1.0f));
        void drawTexture(TextureHandle image, glm::vec2 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.0f));


        void render();

        [[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
        [[nodiscard]] const std::vector<VkDescriptorSet>& getDescriptorSets() const { return m_textureDescriptorSets; }

        TextureHandle createTexture(const std::string& filepath);

        void waitIdle() const;
    private:
        Device* m_device;
        std::unique_ptr<Renderer> m_renderer;

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        uint32_t m_framesInFlight{};

        std::vector<VkDescriptorSet> m_textureDescriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_textureDescriptorSetLayout{};
        std::unique_ptr<Image> m_dummyImage{};

        std::vector<VkDescriptorSet> m_screenSizeDescriptorSets{};
        std::unique_ptr<DescriptorSetLayout> m_screenSizeDescriptorSetLayout{};
        std::vector<std::unique_ptr<Buffer>> m_screenSizeUniformBuffers{};

        VkPipelineLayout m_pipelineLayout{};
        std::unique_ptr<Pipeline> m_pipeline;

        std::unique_ptr<Buffer> m_vertexBuffer{};
        std::unique_ptr<Buffer> m_indexBuffer{};


        std::vector<DrawCommand> m_pendingDrawCommands;

        std::unordered_map<TextureHandle, Texture> m_textures;
        TextureHandle m_nextHandle = 1;
    };
}

#endif //WOMPRENDERER_H
