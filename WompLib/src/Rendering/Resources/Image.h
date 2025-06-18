#ifndef IMAGE_H
#define IMAGE_H

#include <memory>

#include "Buffer.h"
#include "ImageView.h"
#include "Sampler.h"
#include "Rendering/Device.h"

namespace womp {
    class Image {
    public:
        explicit Image(
            Device& device,
            VkExtent2D size,
            VkFormat format,
            VkImageUsageFlags usage,
            VmaMemoryUsage memoryUsage,
            bool createView = true,
            bool createSampler = true,
            VkFilter filter = VK_FILTER_LINEAR
        );
        Image(Device& device, const std::string& filename, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkFilter filter);

        Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage);
        ~Image();


        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView GetImageView() const { return m_imageView->getHandle(); }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }

        VkExtent2D GetExtent() const { return m_extent; }

        [[nodiscard]] VkImageLayout GetCurrentLayout() const { return m_imageLayout; }

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

        VkDescriptorImageInfo descriptorInfo();
        void copyToBuffer(Buffer& buffer, VkExtent2D size) const;


        [[nodiscard]] bool HasStencil() const;
        [[nodiscard]] bool HasDepth() const;

    private:
        void createImage(VkExtent2D size, uint32_t miplevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);
        void createImageView(VkFormat format);
        void createImageSampler(VkFilter filter, VkSamplerAddressMode addressMode);
        static VkImageAspectFlags getImageAspect(VkFormat format);

        Device& m_device;
        VkImage m_image;
        VmaAllocation m_allocation;

        VkExtent2D m_extent{};
        VkImageLayout m_imageLayout{VK_IMAGE_LAYOUT_UNDEFINED};
        VkFormat m_format{VK_FORMAT_UNDEFINED};

        std::unique_ptr<ImageView> m_imageView;
        std::unique_ptr<Sampler> m_sampler;

        bool m_isSwapchainImage{false}; // Indicates if this image is part of the swapchain
    };
}


#endif //IMAGE_H
