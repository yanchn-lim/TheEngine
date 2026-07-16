#include "vulkan_context.hpp"
#include "vulkan_device.hpp"

#include "debug/debug.hpp"

#include <vector>

namespace Graphics
{
    bool VulkanDevice::Init(const VulkanContext& context)
    {
        if (context.GetInstance() == VK_NULL_HANDLE ||
            context.GetSurface() == VK_NULL_HANDLE)
        {
            return false;
        }

        if (!SelectPhysicalDevice(context))
        {
            Shutdown();
            return false;
        }

        if (!CreateLogicalDevice())
        {
            Shutdown();
            return false;
        }

        return true;
    }

    void VulkanDevice::Shutdown()
    {

    }

    VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
    {

    }

    VkDevice VulkanDevice::GetHandle() const
    {

    }

    VkQueue VulkanDevice::GetGraphicsQueue() const
    {

    }

    VkQueue VulkanDevice::GetPresentQueue() const
    {

    }

    uint32_t VulkanDevice::GetGraphicsQueueFamily() const
    {

    }

    uint32_t VulkanDevice::GetPresentQueueFamily() const
    {

    }

    bool VulkanDevice::SelectPhysicalDevice(const VulkanContext& context)
    {
        uint32_t deviceCount;
        VkResult res = vkEnumeratePhysicalDevices(context.GetInstance(), &deviceCount, nullptr);

        if (res != VK_SUCCESS || deviceCount == 0)
        {
            return false;
        }

        std::vector<VkPhysicalDevice> devices;
        res = vkEnumeratePhysicalDevices(context.GetInstance(), &deviceCount, devices.data());


        if (res != VK_SUCCESS)
        {
            return false;
        }

        for (auto& pd : devices)
        {
            if (!IsDeviceSuitable(pd, context.GetSurface()))
            {
                continue;
            }

            _physicalDevice = pd;
            return true;
        }

        return false;
    }

    bool VulkanDevice::CreateLogicalDevice()
    {
        return false;
    }

    bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        return false;
    }

    bool VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        //if (vkGetPhysicalDeviceQueueFamilyProperties())

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

        for (uint32_t i = 0; i < familyCount; ++i)
        {
            const VkQueueFamilyProperties& family = families[i];

        }

        return false;
    }

    bool VulkanDevice::SupportsRequiredExtensions(VkPhysicalDevice device) const
    {
        return false;
    }

    bool VulkanDevice::SupportsRequiredFeatures(VkPhysicalDevice device) const
    {
        return false;
    }
}