# Physical Device and Queues

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

`VulkanDevice` chooses a physical GPU, creates the logical device, and exposes the graphics and presentation queues. It does not own a swapchain, command pool, buffer, or image.

```cpp
class VulkanDevice
{
public:
    bool Init(const VulkanContext& context);
    void Shutdown();

    VkPhysicalDevice GetPhysicalDevice() const;
    VkDevice GetHandle() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;

private:
    VkPhysicalDevice _physicalDevice{ VK_NULL_HANDLE };
    VkDevice _device{ VK_NULL_HANDLE };
    VkQueue _graphicsQueue{ VK_NULL_HANDLE };
    VkQueue _presentQueue{ VK_NULL_HANDLE };
    uint32_t _graphicsQueueFamily{};
    uint32_t _presentQueueFamily{};
};
```

Add a destructor that calls `Shutdown()` and delete copy construction/assignment. A logical device owns GPU resources created through it, so it must outlive swapchains, frames, buffers, images, and pipelines.

## Queue Families

One queue family may support both rendering and presentation, but do not assume it. Discover both capabilities for every candidate device:

```cpp
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool IsComplete() const
    {
        return graphics.has_value() && present.has_value();
    }
};
```

```cpp
VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(
    VkPhysicalDevice device,
    VkSurfaceKHR surface) const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    QueueFamilyIndices indices{};
    for (uint32_t index = 0; index < count; ++index)
    {
        if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            indices.graphics = index;
        }

        VkBool32 supportsPresent = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresent) == VK_SUCCESS &&
            supportsPresent == VK_TRUE)
        {
            indices.present = index;
        }

        if (indices.IsComplete())
        {
            break;
        }
    }

    return indices;
}
```

Do not store queue indices while testing a candidate. Return candidate data, then copy it into members only after the device passes every check.

## Enumerate and Select

Vulkan uses a two-call enumeration pattern: the first call gets a count, and the second fills caller-owned storage.

```cpp
bool VulkanDevice::SelectPhysicalDevice(const VulkanContext& context)
{
    uint32_t count = 0;
    if (vkEnumeratePhysicalDevices(context.GetInstance(), &count, nullptr) != VK_SUCCESS || count == 0)
    {
        return false;
    }

    std::vector<VkPhysicalDevice> devices(count);
    if (vkEnumeratePhysicalDevices(context.GetInstance(), &count, devices.data()) != VK_SUCCESS)
    {
        return false;
    }

    for (VkPhysicalDevice candidate : devices)
    {
        if (!IsDeviceSuitable(candidate, context.GetSurface()))
        {
            continue;
        }

        const QueueFamilyIndices queues = FindQueueFamilies(candidate, context.GetSurface());
        _physicalDevice = candidate;
        _graphicsQueueFamily = *queues.graphics;
        _presentQueueFamily = *queues.present;
        return true;
    }

    return false;
}
```

## Suitability Requirements

Require all of these before selecting a device:

- A graphics queue and a present queue for the current GLFW surface.
- `VK_KHR_swapchain` device extension.
- Vulkan 1.3 `dynamicRendering` and `synchronization2` features.
- At least one supported surface format and one supported present mode.

The feature query belongs on a `VkPhysicalDeviceFeatures2` chain:

```cpp
VkPhysicalDeviceVulkan13Features vulkan13{};
vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

VkPhysicalDeviceFeatures2 features{};
features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
features.pNext = &vulkan13;

vkGetPhysicalDeviceFeatures2(device, &features);
const bool supportsRequiredFeatures =
    vulkan13.dynamicRendering == VK_TRUE &&
    vulkan13.synchronization2 == VK_TRUE;
```

Prefer a discrete GPU only after finding suitable devices. Suitability is mandatory; discrete preference is a scoring decision.

## Verify

Log the selected `VkPhysicalDeviceProperties::deviceName` and the chosen family indices. The engine should initialize and shut down cleanly with the logical-device creation still deferred.

Next: [[03 - Logical Device and Swapchain]].
