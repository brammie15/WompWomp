#include "Image.h"

womp::Image::Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, bool createView, bool createSampler, VkFilter filter)
    : m_device(device), m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE),
      m_format{format}, m_imageView(VK_NULL_HANDLE) {
    createImage(size, 1, format, usage, memoryUsage);
    if (createView) {
        createImageView(format);
    }
    if (createSampler) {
        createImageSampler(filter, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    }
    m_extent = size;

}

womp::Image::Image(Device& device, VkExtent2D size, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VkImage existingImage):
    m_device{device}, m_image(existingImage), m_allocation(VK_NULL_HANDLE), m_extent{size}, m_format{format}, m_imageView(VK_NULL_HANDLE) {
    assert(existingImage != VK_NULL_HANDLE && "Swapchain image is null!");
    createImageView(format);
    m_isSwapchainImage = true; // Mark this image as a swapchain image
}

womp::Image::~Image() {
    if (!m_isSwapchainImage) {
        vmaDestroyImage(m_device.getAllocator(), m_image, m_allocation);
    }
}

void womp::Image::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_imageLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;

    barrier.subresourceRange.aspectMask = 0;
    if (HasDepth()) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencil()) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,  // Use provided source stage mask
        dstStageMask,  // Use provided destination stage mask
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    m_imageLayout = newLayout;
}

VkDescriptorImageInfo womp::Image::descriptorInfo() {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = m_sampler->getHandle();
    imageInfo.imageView = m_imageView->getHandle();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return imageInfo;
}

void womp::Image::createImage(VkExtent2D size, uint32_t miplevels, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = size.height;
    imageInfo.extent.width = size.width;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = miplevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    if (vmaCreateImage(m_device.getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image with VMA!");
    }
}

void womp::Image::createImageView(VkFormat format) {
    m_imageView = std::make_unique<ImageView>(m_device, m_image, format, getImageAspect(format), 1);
}

void womp::Image::createImageSampler(const VkFilter filter, const VkSamplerAddressMode addressMode) {
    m_sampler = std::make_unique<Sampler>(m_device, filter, addressMode, 1);
}

VkImageAspectFlags womp::Image::getImageAspect(const VkFormat format) {
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}