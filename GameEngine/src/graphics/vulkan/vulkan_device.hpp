#pragma once

#include "vulkan_include.hpp"
#include <optional>

namespace Ludus::Graphics
{
    class VulkanContext;

    // selects a physical device and owns its logical device and queues
    class VulkanDevice
    {
    public:
        bool Init(const VulkanContext& context);
        void Shutdown() noexcept;
        void WaitIdle() const;

        // expose Vulkan owners only to the higher-level Vulkan graphics device
        const vk::raii::PhysicalDevice& PhysicalDevice() const { return _physicalDevice; }
        const vk::raii::Device& Device() const { return _device; }
        vk::raii::Device& Device() { return _device; }
        const vk::raii::Queue& GraphicsQueue() const { return _graphicsQueue; }
        const vk::raii::Queue& PresentQueue() const { return _presentQueue; }
        uint32_t GraphicsQueueFamily() const { return _graphicsQueueFamily; }
        uint32_t PresentQueueFamily() const { return _presentQueueFamily; }

    private:
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphics;
            std::optional<uint32_t> present;

            bool Complete() const { return graphics && present; }
        };

        vk::raii::PhysicalDevice _physicalDevice{ nullptr };
        vk::raii::Device _device{ nullptr };
        vk::raii::Queue _graphicsQueue{ nullptr };
        vk::raii::Queue _presentQueue{ nullptr };
        uint32_t _graphicsQueueFamily{};
        uint32_t _presentQueueFamily{};

        QueueFamilyIndices FindQueueFamilies(const vk::raii::PhysicalDevice& candidate, vk::SurfaceKHR surface) const;
        bool IsSuitable(const vk::raii::PhysicalDevice& candidate, vk::SurfaceKHR surface, QueueFamilyIndices& queues) const;
        void SelectPhysicalDevice(const VulkanContext& context);
        void CreateLogicalDevice();
    };
};
