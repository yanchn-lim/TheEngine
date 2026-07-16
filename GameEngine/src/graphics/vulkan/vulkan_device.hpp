#pragma once

#include "vulkan_include.hpp"

namespace Graphics
{
    class VulkanContext;

    class VulkanDevice
    {
    public:
        bool Init(const VulkanContext& context);
        void Shutdown();

        VkPhysicalDevice GetPhysicalDevice() const;
        VkDevice GetHandle() const;
        VkQueue GetGraphicsQueue() const;
        VkQueue GetPresentQueue() const;
        uint32_t GetGraphicsQueueFamily() const;
        uint32_t GetPresentQueueFamily() const;
    private:
        VkPhysicalDevice _physicalDevice{ VK_NULL_HANDLE };
        VkDevice _device{ VK_NULL_HANDLE };
        VkQueue _graphicsQueue{ VK_NULL_HANDLE };
        VkQueue _presentQueue{ VK_NULL_HANDLE };
        uint32_t _graphicsQueueFamily{};
        uint32_t _presentQueueFamily{};

        bool SelectPhysicalDevice(const VulkanContext& context);
        bool CreateLogicalDevice();

        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        bool FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool SupportsRequiredExtensions(VkPhysicalDevice device) const;
        bool SupportsRequiredFeatures(VkPhysicalDevice device) const;
    };
};