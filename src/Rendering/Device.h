#ifndef DEVICE_H
#define DEVICE_H

#include "VkBootstrap.h"
#include "Core/Window.h"

#include <vma/vk_mem_alloc.h>

namespace womp {

    class Device {
    public:
        explicit Device(womp::Window& window);
        ~Device();

        vkb::Device& GetVkbDevice() { return m_device; }
        [[nodiscard]] VkDevice GetVkDevice() const { return m_device.device; }

        [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

        [[nodiscard]] VkCommandPool getCommandPool() const { return m_commandPool; }
        [[nodiscard]] VmaAllocator getAllocator() const { return m_allocator; }

    private:
        void CreateDevice();
        void CreateVma();
        void CreateCommandPool();

        vkb::Instance m_instance{};
        vkb::Device m_device{};
        vkb::PhysicalDevice m_physicalDevice{};
        VkQueue m_graphicsQueue{};

        VkSurfaceKHR m_surface{};

        VmaAllocator m_allocator{};

        VkCommandPool m_commandPool{};

        womp::Window& m_window;
    };
}



#endif //DEVICE_H
