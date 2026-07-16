# Logical Device and Swapchain

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Create the Logical Device

Once a physical device and queue indices are selected, create a logical `VkDevice`. Request one queue from each unique queue family. If graphics and present are the same family, request it only once.

```cpp
bool VulkanDevice::CreateLogicalDevice()
{
    const float priority = 1.0f;
    std::set<uint32_t> uniqueFamilies{
        _graphicsQueueFamily,
        _presentQueueFamily
    };

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (uint32_t family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceVulkan13Features vulkan13{};
    vulkan13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13.dynamicRendering = VK_TRUE;
    vulkan13.synchronization2 = VK_TRUE;

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &vulkan13;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS)
    {
        return false;
    }

    vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, _presentQueueFamily, 0, &_presentQueue);
    return true;
}
```

The feature structure must be chained through `pNext` and must stay alive until `vkCreateDevice` returns. Enabling a feature that was not checked during suitability is a bug.

## Device Shutdown

The caller must destroy all device-owned objects first. Then device shutdown is simple:

```cpp
void VulkanDevice::Shutdown()
{
    if (_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }

    _physicalDevice = VK_NULL_HANDLE;
    _graphicsQueue = VK_NULL_HANDLE;
    _presentQueue = VK_NULL_HANDLE;
    _graphicsQueueFamily = 0;
    _presentQueueFamily = 0;
}
```

Do not call `vkDestroyDevice` while a swapchain, buffer, image, command pool, or pipeline still exists.

## Swapchain Responsibility

`VulkanSwapchain` owns presentation images and their views. It receives `VulkanDevice`, the context surface, and framebuffer dimensions. It does not access GLFW directly except through dimensions given by `VulkanRenderer`.

```cpp
class VulkanSwapchain
{
public:
    bool Create(const VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D requestedExtent);
    bool Recreate(const VulkanDevice& device, VkSurfaceKHR surface, VkExtent2D requestedExtent);
    void Shutdown(const VulkanDevice& device);

    VkSwapchainKHR GetHandle() const;
    VkFormat GetFormat() const;
    VkExtent2D GetExtent() const;
    VkImage GetImage(uint32_t index) const;
    VkImageView GetImageView(uint32_t index) const;
};
```

## Choose Surface Settings

Query capabilities, formats, and present modes before creating a swapchain. Keep them in a value type local to this class.

```cpp
struct SwapchainSupport
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

Choose `VK_FORMAT_B8G8R8A8_SRGB` with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` when present. Prefer `VK_PRESENT_MODE_MAILBOX_KHR`; use `VK_PRESENT_MODE_FIFO_KHR` otherwise because it is guaranteed. Use the surface's `currentExtent` when it is fixed; otherwise clamp the GLFW framebuffer size to `minImageExtent` and `maxImageExtent`.

```cpp
VkExtent2D VulkanSwapchain::ChooseExtent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    VkExtent2D requestedExtent) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    return {
        std::clamp(requestedExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(requestedExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}
```

Create one `VkImageView` for every swapchain image. The images themselves are owned by the swapchain and must not be destroyed manually. The image views are owned by the engine and must be destroyed before the swapchain.

## Resize and Minimize

The GLFW resize callback only sets `_resizePending`. At the start of a safe frame boundary, call `glfwGetFramebufferSize`; if either dimension is zero, return without recreating. A zero extent is normal while a window is minimized.

Recreation must wait for all current device work first, destroy format/extent-dependent resources, create the new swapchain and views, then rebuild those resources. Begin with `vkDeviceWaitIdle`; optimize later only after the basic path is correct.

## Verify

Create the swapchain and image views, then destroy them without rendering. Resize, minimize, restore, and close the window. Validation must remain clean.

Next: [[04 - Frames and Synchronization]].
