#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

#include "debug/debug.hpp"

#include <vector>
#include <optional>

namespace Graphics
{
    bool VulkanDevice::Init(const VulkanContext& context)
    {
        Shutdown();
        try
        {
            SelectPhysicalDevice(context);
            CreateLogicalDevice();
            return true;
        }
        catch (const std::exception& error)
        {
            Debug::LogError("VulkanDevice::Init: ", error.what());
            Shutdown();
            return false;
        }
    }

    void VulkanDevice::Shutdown() noexcept
    {
        _presentQueue = nullptr;
        _graphicsQueue = nullptr;
        _device = nullptr;
        _physicalDevice = nullptr;
        _graphicsQueueFamily = 0;
        _presentQueueFamily = 0;
    }

    void VulkanDevice::WaitIdle() const
    {
        if (*_device)
            _device.waitIdle();
    }

    QueueFamilyIndices VulkanDevice::FindQueueFamilies(const vk::raii::PhysicalDevice& candidate, vk::SurfaceKHR surface) const
    {
        QueueFamilyIndices result;
        const auto families = candidate.getQueueFamilyProperties();

        for (uint32_t index = 0; index < families.size(); ++index)
        {
            if (families[index].queueFlags & vk::QueueFlagBits::eGraphics)
                result.graphics = index;

            if (candidate.getSurfaceSupportKHR(index, surface))
                result.present = index;

            if (result.Complete())
                break;
        }

        return result;
    }

    bool VulkanDevice::IsSuitable(const vk::raii::PhysicalDevice& candidate, vk::SurfaceKHR surface, QueueFamilyIndices& queues) const
    {
        //device must provide vulkan 1.3
        const vk::PhysicalDeviceProperties properties = candidate.getProperties();
        if (properties.apiVersion < VK_API_VERSION_1_3)
            return false;

        queues = FindQueueFamilies(candidate, surface);
        if (!queues.Complete())
            return false;

        //check for swapchain extension
        bool hasSwapchain = false;
        for (const vk::ExtensionProperties& extension : candidate.enumerateDeviceExtensionProperties())
        {
            if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            {
                hasSwapchain = true;
                break;
            }
        }

        if (!hasSwapchain)
            return false;

        //check for vulkan 1.3 features
        const auto featureChain = candidate.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
        const auto& vulkan13 = featureChain.get<vk::PhysicalDeviceVulkan13Features>();
        if (!vulkan13.dynamicRendering || !vulkan13.synchronization2)
            return false;

        return  !candidate.getSurfaceFormatsKHR(surface).empty() &&
                !candidate.getSurfacePresentModesKHR(surface).empty();
    }

    void VulkanDevice::SelectPhysicalDevice(const VulkanContext& context)
    {
        for (const vk::raii::PhysicalDevice& candidate : context.Instance().enumeratePhysicalDevices())
        {
            //only queue indices suitable
            QueueFamilyIndices queues;
            if (!IsSuitable(candidate, context.SurfaceHandle(), queues))
                continue;

            _physicalDevice = candidate;
            _graphicsQueueFamily = *queues.graphics;
            _presentQueueFamily = *queues.present;

            const auto properties = candidate.getProperties();
            Debug::CLog("Selected Vulkan device: ", properties.deviceName.data(), "\n");
            return;
        }

        throw std::runtime_error("No suitable Vulkan 1.3 device found");
    }

    //get each family and enable features
    void VulkanDevice::CreateLogicalDevice()
    {
        const float priority = 1.0f;
        std::vector<uint32_t> families{ _graphicsQueueFamily };
        if (_presentQueueFamily != _graphicsQueueFamily)
            families.push_back(_presentQueueFamily);

        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        queueInfos.reserve(families.size());
        for (uint32_t family : families)
        {
            vk::DeviceQueueCreateInfo info{};
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            info.pQueuePriorities = &priority;
            queueInfos.push_back(info);
        }

        vk::PhysicalDeviceVulkan13Features vulkan13{};
        vulkan13.synchronization2 = vk::True;
        vulkan13.dynamicRendering = vk::True;

        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan13Features> features{
                vk::PhysicalDeviceFeatures2{}, vulkan13
        };

        constexpr std::array extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        vk::DeviceCreateInfo createInfo{};
        createInfo.pNext = &features.get<vk::PhysicalDeviceFeatures2>();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        _device = vk::raii::Device(_physicalDevice, createInfo);
        _graphicsQueue = vk::raii::Queue(_device, _graphicsQueueFamily, 0);
        _presentQueue = vk::raii::Queue(_device, _presentQueueFamily, 0);
    }

}