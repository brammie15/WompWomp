#include "Buffer.h"

namespace womp {
    Buffer::Buffer(Device& deviceRef, VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable): m_device{deviceRef} {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;

        if (mappable) {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        } else {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        m_size = size;

        createBuffer(size, usageFlags, memoryUsage, mappable);
    }

    Buffer::~Buffer() {
        if (m_data != nullptr && !m_mappedViaCreateFlag) {
            unmap();
        }
        vmaDestroyBuffer(m_device.getAllocator(), m_buffer, m_allocation);
    }

    VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
        if (m_data == nullptr) {
            return vmaMapMemory(m_device.getAllocator(), m_allocation, &m_data);
        }
        return VK_SUCCESS;
    }

    void Buffer::unmap() {
        if (m_data != nullptr && !m_mappedViaCreateFlag) {
            vmaUnmapMemory(m_device.getAllocator(), m_allocation);
            m_data = nullptr;
        }
    }

    void Buffer::copyTo(const void* data, VkDeviceSize size) const {
        // assert(m_data != nullptr && "Cannot copy to buffer if buffer is not mapped");
        // memcpy(m_data, data, size);
        vmaCopyMemoryToAllocation(m_device.getAllocator(), data, m_allocation, 0, size);
    }

    VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
        return VkDescriptorBufferInfo{
            m_buffer,
            offset,
            size
        };
    }

    void Buffer::flush() const {
        vmaFlushAllocation(m_device.getAllocator(), m_allocation, 0, VK_WHOLE_SIZE);
    }

    void Buffer::copyToBuffer(Buffer* dstBuffer, uint32_t size) {
        const VkCommandBuffer commandBuffer = m_device.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        copyRegion.srcOffset = 0;
        vkCmdCopyBuffer(commandBuffer, m_buffer, dstBuffer->getBuffer(), 1, &copyRegion);

        m_device.endSingleTimeCommands(commandBuffer);
    }

    void Buffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, bool mappable) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = usageFlags;
        bufferInfo.size = size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationInfo{};
        allocationInfo.usage = memoryUsage;
        allocationInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        if (mappable) {
            allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;
            m_mappedViaCreateFlag = true;
        } else {
            allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            allocationInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            m_mappedViaCreateFlag = false;
        }


        VmaAllocationInfo allocInfo{};
        if (vmaCreateBuffer(m_device.getAllocator(), &bufferInfo, &allocationInfo, &m_buffer, &m_allocation, &allocInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        // Optionally save the mapped pointer directly if you used VMA_ALLOCATION_CREATE_MAPPED_BIT
        if (mappable) {
            m_data = allocInfo.pMappedData;
        }
    }
}
