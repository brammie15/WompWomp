#ifndef IMAGE_H
#define IMAGE_H

#include <memory>

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

        Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage);
        ~Image();


        [[nodiscard]] VkImage getImage() const { return m_image; }
        [[nodiscard]] VkImageView GetImageView() const { return m_imageView->getHandle(); }
        [[nodiscard]] VmaAllocation getAllocation() const { return m_allocation; }

        VkExtent2D GetExtent() const { return m_extent; }


        [[nodiscard]] VkImageLayout GetCurrentLayout() const { return m_imageLayout; }

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout,
                                 VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

        VkDescriptorImageInfo descriptorInfo();


        [[nodiscard]] bool HasStencil() const {
            switch (m_format)
            {
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] bool HasDepth() const {
            switch (m_format)
            {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return true;
                default:
                    return false;
            }
        }


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
