#include "Device.h"

#include <iostream>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "DebugLabel.h"

namespace womp {
    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = nullptr;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR = nullptr;
}

womp::Device::Device(womp::Window& window) : m_window(window) {
    CreateDevice();
    CreateVma();
    CreateCommandPool();
}

womp::Device::~Device() {
    vmaDestroyAllocator(m_allocator); //Thanks thalia <3
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkb::destroy_surface(m_instance, m_surface);
    vkb::destroy_device(m_device);
    vkb::destroy_instance(m_instance);
}

VkFormat womp::Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
    for (const VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (
            tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void womp::Device::CreateDevice() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder
    .set_app_name("Dingus")
    .request_validation_layers()
    .use_default_debug_messenger()
    .enable_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
    .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME)
    .require_api_version(1,3)
    .set_minimum_instance_version(1, 3)
    .build();

    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance: " << inst_ret.error().message() << std::endl;
    }

    m_instance = inst_ret.value();

    m_surface = m_window.createVulkanSurface(m_instance.instance);

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features{};
    dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamic_rendering_features.dynamicRendering = VK_TRUE;


    vkb::PhysicalDeviceSelector selector{ m_instance };
    auto phys_ret = selector
        .set_minimum_version(1, 3)
        .set_surface(m_surface)
        .require_present(true)
        .set_required_features(device_features)
        .add_required_extension_features(dynamic_rendering_features)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
        .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
        .select();

    if (!phys_ret) {
        std::cerr << "Failed to select physical device: " << phys_ret.error().message() << std::endl;
    }

    m_physicalDevice = phys_ret.value();

    vkb::DeviceBuilder device_builder{ phys_ret.value() };



    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create Vulkan device: " << dev_ret.error().message() << std::endl;
    }

    m_device = std::move(dev_ret.value());

    auto graphics_queue_ret = m_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        std::cerr << "Failed to get graphics queue. Error: " << graphics_queue_ret.error().message() << "\n";
    }
    m_graphicsQueue = graphics_queue_ret.value();

    //
    //
    // vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderingKHR"));
    // vkCmdEndRenderingKHR   = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR"));
    //
    // if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR) {
    //     throw std::runtime_error("Failed to load VK_KHR_dynamic_rendering function pointers.");
    // }

    DebugLabel::Init(m_device);
}

void womp::Device::CreateVma() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.physicalDevice = m_device.physical_device;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;


    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        throw std::runtime_error("failed to create VMA allocator!");
    }
}

void womp::Device::CreateCommandPool() {
    const uint32_t graphicsQueueFamilyIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_device.device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}
