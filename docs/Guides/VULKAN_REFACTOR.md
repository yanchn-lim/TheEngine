# Vulkan RAII Refactor

This guide replaces the backend's raw Vulkan C ownership with header-based Vulkan-Hpp RAII while preserving the engine's existing `Graphics::IRenderer` boundary and frame loop. Apply the stages in order and keep OpenGL as the default until the Vulkan clear-frame checkpoint passes.

The target remains Vulkan 1.3 with dynamic rendering, Synchronization 2, `VK_KHR_swapchain`, GLFW, and the existing Visual Studio project. Do not copy the Khronos tutorial's single application class, Vulkan 1.4 baseline, Slang shaders, or experimental C++ module into this engine. Use the new tutorial for Vulkan mechanics, then place those mechanics inside the owners described here.

This document is the source of truth for Vulkan-Hpp ownership. The existing chaptered Vulkan implementation guide remains useful for engine milestones, but its raw `Vk*` snippets must not be combined with the RAII ownership shown here.

Official references:

- [Khronos Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/00_Introduction.html)
- [Vulkan-Hpp repository](https://github.com/KhronosGroup/Vulkan-Hpp)
- [GLFW surface wrapped by `vk::raii::SurfaceKHR`](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html)
- [Swapchain recreation and `VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS`](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html)

## Final Shape

Keep this application-level flow:

```text
Engine
  -> Graphics::IRenderer
       -> OpenGLRenderer
       -> VulkanRenderer
            -> VulkanContext
            -> VulkanDevice
            -> VulkanSwapchain
            -> VulkanFrameResources[2]
            -> Vulkan pipelines and resources
```

`VulkanRenderer` adapts the explicit Vulkan frame model to the current engine lifecycle:

```text
BeginFrame()
  wait, acquire, begin command buffer, begin dynamic rendering

Submit(DrawCmd) ...
  copy backend-neutral commands into the frame command list

EndFrame()
  resolve Vulkan resources and record world draws

ImGui overlay
  record ImGui into the still-active dynamic-rendering scope

Present()
  end rendering, transition, submit, present, advance frame
```

RAII changes ownership, not architecture. Keep engine-level aggregate classes such as `VulkanDevice` and `VulkanSwapchain`, but do not add another one-object wrapper around every `vk::raii::*` handle. A `VulkanBuffer` remains useful because it groups a buffer, memory, size, and usage; a class that only forwards `vk::raii::Fence` is not useful.

## Stage 0: Freeze the Policies

Use these rules throughout the refactor:

1. Only Vulkan implementation files may include Vulkan-Hpp types. `DrawCmd`, Scene, assets, importers, and backend-neutral graphics headers must not contain `vk::*` or `Vk*` types.
2. Use owning `vk::raii::*` handles for objects the engine creates and destroys. Use non-owning `vk::*` handles for swapchain images, formats, extents, and temporary command arguments.
3. Keep exceptions enabled inside Vulkan-Hpp. Catch `vk::SystemError` and `std::exception` at engine boundaries such as `VulkanRenderer::Init`; do not let an exception escape into `Engine::Run`.
4. Keep explicit `Shutdown()` methods because `IRenderer` requires them. Implement them by clearing RAII owners in dependency order; destructors remain the fallback.
5. Declare owners before their dependants because C++ destroys members in reverse declaration order.
6. Do not mix raw `vkCreate*`/`vkDestroy*` ownership with RAII ownership. GLFW and Dear ImGui require raw Vulkan handles at their C boundaries; unwrap immediately before the call and wrap returned owned handles immediately afterward.
7. Require Vulkan 1.3. Using headers from a Vulkan 1.4 SDK does not change the requested runtime API version.

Do not disable exceptions with `VULKAN_HPP_NO_EXCEPTIONS` during this migration. RAII constructors cannot report creation failure through the engine's `bool Init()` convention without either exceptions or a second layer of manual result handling.

## Stage 1: Update the Build and Centralize Vulkan-Hpp Configuration

Change both `LanguageStandard` entries in `GameEngine/GameEngine.vcxproj` from `stdcpp17` to `stdcpp20`. Keep `$(VULKAN_SDK)\Include`, `$(VULKAN_SDK)\Lib`, and `vulkan-1.lib` as they are.

Add `GameEngine/src/graphics/vulkan/vulkan_include.hpp`:

```cpp
#pragma once

// These definitions must be identical before every Vulkan-Hpp include.
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <vulkan/vulkan_raii.hpp>
```

`VULKAN_HPP_NO_STRUCT_CONSTRUCTORS` makes Vulkan structures aggregates, so the C++20 designated initializers used by the new tutorial work. `VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS` lets acquire and present return `vk::Result::eErrorOutOfDateKHR` instead of throwing before the renderer can schedule swapchain recreation.

Every engine Vulkan file must include `vulkan_include.hpp`; remove direct engine includes of `<vulkan/vulkan.h>`, `<vulkan/vulkan.hpp>`, and `<vulkan/vulkan_raii.hpp>`. Do not put this header into `pch.hpp`, because that would make all engine translation units parse Vulkan-Hpp.

Include standard-library dependencies in the header that uses them. In particular, add `<optional>` for queue discovery, `<span>` for SPIR-V views, and `<array>`/`<vector>` for frame and resource storage instead of relying on the forced precompiled header.

Add every new `.cpp` and `.hpp` to `GameEngine.vcxproj` and `GameEngine.vcxproj.filters`. At minimum the RAII path will contain:

```text
graphics/vulkan/
  vulkan_include.hpp
  vulkan_context.*
  vulkan_device.*
  vulkan_swapchain.*
  vulkan_frame_resources.hpp
  vulkan_renderer.*
  vulkan_buffer.*
  vulkan_image.*
  vulkan_pipeline.*
```

First checkpoint: build OpenGL in Debug and Release before changing any Vulkan class. This catches a C++20 or project-file regression independently of Vulkan.

## Stage 2: Convert `VulkanContext`

`VulkanContext` owns instance-level objects only. Declare `_loader` first, then the instance, debug messenger, and surface so destruction runs as surface, messenger, instance, loader.

Replace `vulkan_context.hpp` with this shape:

```cpp
#pragma once

#include "graphics/vulkan/vulkan_include.hpp"

struct GLFWwindow;

namespace Graphics
{
    class VulkanContext
    {
    public:
        VulkanContext() = default;
        ~VulkanContext();

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        bool Init(GLFWwindow* window);
        void Shutdown() noexcept;

        const vk::raii::Instance& Instance() const { return _instance; }
        vk::Instance InstanceHandle() const { return *_instance; }
        vk::SurfaceKHR SurfaceHandle() const { return *_surface; }

    private:
        bool ValidationLayerAvailable() const;
        void CreateInstance();
        void CreateDebugMessenger();
        void CreateSurface(GLFWwindow* window);

        vk::raii::Context _loader;
        vk::raii::Instance _instance{ nullptr };
        vk::raii::DebugUtilsMessengerEXT _debugMessenger{ nullptr };
        vk::raii::SurfaceKHR _surface{ nullptr };
    };
}
```

Keep the callback as a free function because Vulkan stores a C function pointer:

```cpp
namespace
{
    constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*)
    {
        const char* message = callbackData && callbackData->pMessage
            ? callbackData->pMessage
            : "Unknown Vulkan validation message";

        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            Debug::LogError("Vulkan validation: ", message);
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            Debug::LogWarning("Vulkan validation: ", message);
        else
            Debug::LogVerbose("Vulkan validation: ", message);

        return VK_FALSE;
    }

    vk::DebugUtilsMessengerCreateInfoEXT MakeDebugMessengerInfo()
    {
        return {
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                               vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = DebugCallback
        };
    }
}
```

Wrap the complete initialization sequence in one exception boundary:

```cpp
bool Graphics::VulkanContext::Init(GLFWwindow* window)
{
    Shutdown();

    if (!window || glfwVulkanSupported() != GLFW_TRUE)
    {
        Debug::LogError("VulkanContext::Init: Vulkan or the GLFW window is unavailable");
        return false;
    }

    try
    {
#if defined(_DEBUG)
        if (!ValidationLayerAvailable())
            throw std::runtime_error("VK_LAYER_KHRONOS_validation is unavailable");
#endif

        CreateInstance();
        CreateDebugMessenger();
        CreateSurface(window);
        return true;
    }
    catch (const vk::SystemError& error)
    {
        Debug::LogError("VulkanContext::Init: ", error.what());
    }
    catch (const std::exception& error)
    {
        Debug::LogError("VulkanContext::Init: ", error.what());
    }

    Shutdown();
    return false;
}
```

Vulkan-Hpp replaces manual two-call enumeration for layers:

```cpp
bool Graphics::VulkanContext::ValidationLayerAvailable() const
{
    for (const vk::LayerProperties& layer : _loader.enumerateInstanceLayerProperties())
    {
        if (std::strcmp(layer.layerName, ValidationLayer) == 0)
            return true;
    }
    return false;
}
```

Create the Vulkan 1.3 instance, retaining GLFW's required extensions:

```cpp
void Graphics::VulkanContext::CreateInstance()
{
    uint32_t extensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!glfwExtensions || extensionCount == 0)
        throw std::runtime_error("GLFW returned no Vulkan instance extensions");

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);
    std::vector<const char*> layers;

#if defined(_DEBUG)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.push_back(ValidationLayer);
#endif

    const vk::ApplicationInfo applicationInfo{
        .pApplicationName = "TheEngine",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "TheEngine",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = MakeDebugMessengerInfo();
    const vk::InstanceCreateInfo createInfo{
#if defined(_DEBUG)
        .pNext = &debugInfo,
#endif
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    _instance = vk::raii::Instance(_loader, createInfo);
}
```

Create the debug messenger only in Debug. The RAII instance dispatcher loads `VK_EXT_debug_utils`; the old `vkGetInstanceProcAddr` helper is no longer needed.

```cpp
void Graphics::VulkanContext::CreateDebugMessenger()
{
#if defined(_DEBUG)
    _debugMessenger = vk::raii::DebugUtilsMessengerEXT(
        _instance,
        MakeDebugMessengerInfo());
#endif
}
```

GLFW returns a raw surface because it exposes the C API. Wrap it immediately so exactly one RAII owner destroys it:

```cpp
void Graphics::VulkanContext::CreateSurface(GLFWwindow* window)
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(
        static_cast<VkInstance>(*_instance),
        window,
        nullptr,
        &rawSurface);

    if (result != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface failed");

    _surface = vk::raii::SurfaceKHR(_instance, rawSurface);
}
```

`Shutdown()` is short and idempotent:

```cpp
Graphics::VulkanContext::~VulkanContext()
{
    Shutdown();
}

void Graphics::VulkanContext::Shutdown() noexcept
{
    _surface = nullptr;
    _debugMessenger = nullptr;
    _instance = nullptr;
}
```

Checkpoint: select Vulkan temporarily. A no-API GLFW window must open and close with validation clean. Restore OpenGL as the default.

## Stage 3: Finish `VulkanDevice` with RAII

`vk::raii::PhysicalDevice` and `vk::raii::Queue` are non-destroying wrappers, while `vk::raii::Device` owns the logical device. The instance must outlive all three.

Use this class shape:

```cpp
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool Complete() const { return graphics && present; }
};

class VulkanDevice
{
public:
    bool Init(const VulkanContext& context);
    void Shutdown() noexcept;
    void WaitIdle() const;

    const vk::raii::PhysicalDevice& PhysicalDevice() const { return _physicalDevice; }
    const vk::raii::Device& Device() const { return _device; }
    vk::raii::Device& Device() { return _device; }
    const vk::raii::Queue& GraphicsQueue() const { return _graphicsQueue; }
    const vk::raii::Queue& PresentQueue() const { return _presentQueue; }
    uint32_t GraphicsQueueFamily() const { return _graphicsQueueFamily; }
    uint32_t PresentQueueFamily() const { return _presentQueueFamily; }

private:
    QueueFamilyIndices FindQueueFamilies(
        const vk::raii::PhysicalDevice& candidate,
        vk::SurfaceKHR surface) const;
    bool IsSuitable(
        const vk::raii::PhysicalDevice& candidate,
        vk::SurfaceKHR surface,
        QueueFamilyIndices& queues) const;
    void SelectPhysicalDevice(const VulkanContext& context);
    void CreateLogicalDevice();

    vk::raii::PhysicalDevice _physicalDevice{ nullptr };
    vk::raii::Device _device{ nullptr };
    vk::raii::Queue _graphicsQueue{ nullptr };
    vk::raii::Queue _presentQueue{ nullptr };
    uint32_t _graphicsQueueFamily = 0;
    uint32_t _presentQueueFamily = 0;
};
```

Find graphics and presentation independently; do not assume one family supports both:

```cpp
QueueFamilyIndices VulkanDevice::FindQueueFamilies(
    const vk::raii::PhysicalDevice& candidate,
    vk::SurfaceKHR surface) const
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
```

A suitable device must provide Vulkan 1.3, the swapchain extension, both required Vulkan 1.3 features, queue support, formats, and present modes:

```cpp
bool VulkanDevice::IsSuitable(
    const vk::raii::PhysicalDevice& candidate,
    vk::SurfaceKHR surface,
    QueueFamilyIndices& queues) const
{
    const vk::PhysicalDeviceProperties properties = candidate.getProperties();
    if (properties.apiVersion < VK_API_VERSION_1_3)
        return false;

    queues = FindQueueFamilies(candidate, surface);
    if (!queues.Complete())
        return false;

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

    const auto featureChain = candidate.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features>();
    const auto& vulkan13 = featureChain.get<vk::PhysicalDeviceVulkan13Features>();
    if (!vulkan13.dynamicRendering || !vulkan13.synchronization2)
        return false;

    return !candidate.getSurfaceFormatsKHR(surface).empty() &&
           !candidate.getSurfacePresentModesKHR(surface).empty();
}
```

Enumerate through the RAII instance and commit queue indices only after a candidate passes every test:

```cpp
void VulkanDevice::SelectPhysicalDevice(const VulkanContext& context)
{
    for (const vk::raii::PhysicalDevice& candidate : context.Instance().enumeratePhysicalDevices())
    {
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
```

Request each unique queue family once and enable only features already checked:

```cpp
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
        queueInfos.push_back({
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &priority
        });
    }

    vk::PhysicalDeviceVulkan13Features vulkan13{
        .synchronization2 = vk::True,
        .dynamicRendering = vk::True
    };
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features> features{
            vk::PhysicalDeviceFeatures2{}, vulkan13
        };

    constexpr std::array extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const vk::DeviceCreateInfo createInfo{
        .pNext = &features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos = queueInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    _device = vk::raii::Device(_physicalDevice, createInfo);
    _graphicsQueue = vk::raii::Queue(_device, _graphicsQueueFamily, 0);
    _presentQueue = vk::raii::Queue(_device, _presentQueueFamily, 0);
}
```

`Init()` catches errors exactly as `VulkanContext::Init()` does:

```cpp
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

void VulkanDevice::WaitIdle() const
{
    if (*_device)
        _device.waitIdle();
}
```

`Shutdown()` assumes the renderer has already destroyed every device child:

```cpp
void VulkanDevice::Shutdown() noexcept
{
    _presentQueue = nullptr;
    _graphicsQueue = nullptr;
    _device = nullptr;
    _physicalDevice = nullptr;
    _graphicsQueueFamily = 0;
    _presentQueueFamily = 0;
}
```

Checkpoint: log the selected GPU and queue indices, then shut down without creating a swapchain. Validation must remain clean.

## Stage 4: Add the RAII Swapchain

`VulkanSwapchain` owns the swapchain and image views. It stores swapchain images as non-owning `vk::Image` values because the presentation engine owns them.

```cpp
class VulkanSwapchain
{
public:
    bool Create(
        const VulkanDevice& device,
        vk::SurfaceKHR surface,
        vk::Extent2D requestedExtent);
    bool Recreate(
        const VulkanDevice& device,
        vk::SurfaceKHR surface,
        vk::Extent2D requestedExtent);
    void Shutdown() noexcept;

    const vk::raii::SwapchainKHR& Swapchain() const { return _swapchain; }
    vk::Format Format() const { return _format; }
    vk::Extent2D Extent() const { return _extent; }
    vk::Image Image(uint32_t index) const { return _images.at(index); }
    vk::ImageView ImageView(uint32_t index) const { return *_imageViews.at(index); }
    uint32_t ImageCount() const { return static_cast<uint32_t>(_images.size()); }

private:
    vk::raii::SwapchainKHR _swapchain{ nullptr };
    std::vector<vk::Image> _images;
    std::vector<vk::raii::ImageView> _imageViews;
    vk::Format _format = vk::Format::eUndefined;
    vk::Extent2D _extent{};
};
```

Query support through `VulkanDevice::PhysicalDevice()`:

```cpp
const auto capabilities = device.PhysicalDevice().getSurfaceCapabilitiesKHR(surface);
const auto formats = device.PhysicalDevice().getSurfaceFormatsKHR(surface);
const auto presentModes = device.PhysicalDevice().getSurfacePresentModesKHR(surface);
```

Select `vk::Format::eB8G8R8A8Srgb` plus `vk::ColorSpaceKHR::eSrgbNonlinear` when available. Prefer `vk::PresentModeKHR::eMailbox`; fall back to `eFifo`. If `capabilities.currentExtent.width != UINT32_MAX`, use it; otherwise clamp the requested framebuffer extent to the surface limits.

Create the swapchain with concurrent sharing only when the queue families differ:

```cpp
const uint32_t queueFamilies[] = {
    device.GraphicsQueueFamily(),
    device.PresentQueueFamily()
};
const bool separateQueues = queueFamilies[0] != queueFamilies[1];

vk::SwapchainCreateInfoKHR createInfo{
    .surface = surface,
    .minImageCount = imageCount,
    .imageFormat = selectedFormat.format,
    .imageColorSpace = selectedFormat.colorSpace,
    .imageExtent = selectedExtent,
    .imageArrayLayers = 1,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
    .imageSharingMode = separateQueues ? vk::SharingMode::eConcurrent
                                       : vk::SharingMode::eExclusive,
    .queueFamilyIndexCount = separateQueues ? 2u : 0u,
    .pQueueFamilyIndices = separateQueues ? queueFamilies : nullptr,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    .presentMode = selectedPresentMode,
    .clipped = vk::True
};

_swapchain = vk::raii::SwapchainKHR(device.Device(), createInfo);
_images = _swapchain.getImages();
_format = selectedFormat.format;
_extent = selectedExtent;

_imageViews.clear();
_imageViews.reserve(_images.size());
for (vk::Image image : _images)
{
    const vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = _format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    _imageViews.emplace_back(device.Device(), viewInfo);
}
```

Start with a correctness-first recreation path:

```cpp
bool VulkanSwapchain::Recreate(
    const VulkanDevice& device,
    vk::SurfaceKHR surface,
    vk::Extent2D requestedExtent)
{
    device.WaitIdle();
    Shutdown();
    return Create(device, surface, requestedExtent);
}

void VulkanSwapchain::Shutdown() noexcept
{
    _imageViews.clear();
    _images.clear();
    _swapchain = nullptr;
    _format = vk::Format::eUndefined;
    _extent = {};
}
```

The views must be destroyed before the swapchain. Optimize recreation with `oldSwapchain` only after resize, minimize, restore, and shutdown are validation-clean.

Checkpoint: create and destroy the swapchain without rendering. Exercise resize and minimize; a zero framebuffer extent must postpone recreation rather than fail initialization.

## Stage 5: Add Two RAII Frames in Flight

Each frame owns the command pool, command buffer, fence, and binary semaphores used by acquire and present:

Keep the acquire and render-finished semaphores binary even though the new tutorial also teaches timeline semaphores. Window-system acquire and present use binary semaphores; timeline semaphores can be added later for engine-internal queue scheduling.

```cpp
struct VulkanFrameResources
{
    // Declaration order gives: semaphores, fence, command buffer, command pool.
    vk::raii::CommandPool commandPool{ nullptr };
    vk::raii::CommandBuffer commandBuffer{ nullptr };
    vk::raii::Fence inFlightFence{ nullptr };
    vk::raii::Semaphore imageAvailable{ nullptr };
    vk::raii::Semaphore renderFinished{ nullptr };
};

inline constexpr uint32_t FramesInFlight = 2;
```

Initialize each frame after the device and swapchain exist:

```cpp
void VulkanRenderer::CreateFrameResources()
{
    for (VulkanFrameResources& frame : _frames)
    {
        frame.commandPool = vk::raii::CommandPool(
            _device.Device(),
            vk::CommandPoolCreateInfo{
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = _device.GraphicsQueueFamily()
            });

        auto commandBuffers = _device.Device().allocateCommandBuffers(
            vk::CommandBufferAllocateInfo{
                .commandPool = *frame.commandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1
            });
        frame.commandBuffer = std::move(commandBuffers.front());

        frame.inFlightFence = vk::raii::Fence(
            _device.Device(),
            vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
        frame.imageAvailable = vk::raii::Semaphore(
            _device.Device(), vk::SemaphoreCreateInfo{});
        frame.renderFinished = vk::raii::Semaphore(
            _device.Device(), vk::SemaphoreCreateInfo{});
    }
}
```

Declare `VulkanRenderer` members in dependency order:

```cpp
class VulkanRenderer final : public IRenderer
{
    GLFWwindow* _window = nullptr;
    VulkanContext _context;
    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    std::array<VulkanFrameResources, FramesInFlight> _frames;

    std::vector<DrawCmd> _commands;
    Camera2D _camera;
    uint32_t _frameIndex = 0;
    uint32_t _imageIndex = 0;
    bool _frameReady = false;
    bool _renderingActive = false;
    bool _resizePending = false;
    bool _fatalError = false;
    vk::Extent2D _requestedExtent{};
};
```

Any pipeline, descriptor, buffer, image, or ImGui owner must be declared after `_frames` so it is destroyed before the device. Explicit `Shutdown()` must still clear everything in the correct order.

Wire the existing `IRenderer` entry points before starting the frame loop:

```cpp
bool VulkanRenderer::Init(GLFWwindow* window)
{
    Shutdown();
    _window = window;
    _commands.reserve(8192);

    if (!_window || !_context.Init(_window) || !_device.Init(_context))
    {
        Shutdown();
        return false;
    }

    try
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(_window, &width, &height);
        if (width <= 0 || height <= 0)
            throw std::runtime_error("Initial Vulkan framebuffer extent is zero");

        _requestedExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        if (!_swapchain.Create(_device, _context.SurfaceHandle(), _requestedExtent))
            throw std::runtime_error("Vulkan swapchain creation failed");

        CreateFrameResources();
        return true;
    }
    catch (const std::exception& error)
    {
        Debug::LogError("VulkanRenderer::Init: ", error.what());
        Shutdown();
        return false;
    }
}

void VulkanRenderer::Submit(const DrawCmd& command)
{
    if (_frameReady)
        _commands.push_back(command);
}

void VulkanRenderer::SetCamera(const Camera2D& camera)
{
    _camera = camera;
}

void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
{
    _requestedExtent = { width, height };
    _resizePending = true;
}
```

`OnResize()` must only record the request. Recreating from a GLFW callback or halfway through command recording breaks frame ownership.

Acquire before resetting the fence. If acquisition reports out-of-date and the fence was already reset, the frame can deadlock forever because nothing will signal it.

```cpp
void VulkanRenderer::BeginFrame()
{
    _commands.clear();
    _frameReady = false;

    if (_fatalError || !TryRecreateSwapchain())
        return;

    try
    {
        VulkanFrameResources& frame = _frames[_frameIndex];
        const vk::Result waitResult = _device.Device().waitForFences(
            *frame.inFlightFence, vk::True, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan frame fence wait failed");

        const auto [acquireResult, imageIndex] = _swapchain.Swapchain().acquireNextImage(
            UINT64_MAX, *frame.imageAvailable, nullptr);

        if (acquireResult == vk::Result::eErrorOutOfDateKHR)
        {
            _resizePending = true;
            return;
        }
        if (acquireResult != vk::Result::eSuccess &&
            acquireResult != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Vulkan swapchain acquisition failed");
        }

        _resizePending |= acquireResult == vk::Result::eSuboptimalKHR;
        _imageIndex = imageIndex;

        _device.Device().resetFences(*frame.inFlightFence);
        frame.commandPool.reset({});
        frame.commandBuffer.begin(vk::CommandBufferBeginInfo{});
        _frameReady = true;
    }
    catch (const std::exception& error)
    {
        Debug::LogError("VulkanRenderer::BeginFrame: ", error.what());
        _fatalError = true;
    }
}
```

`TryRecreateSwapchain()` checks `_resizePending`, calls `glfwGetFramebufferSize`, returns `false` while either dimension is zero, waits idle, recreates the swapchain plus every extent/format-dependent resource, and clears `_resizePending` only after success.

Checkpoint: begin and end command buffers without submitting yet. Validation should report no command-pool or fence lifetime errors.

## Stage 6: Record a Clear with Dynamic Rendering

Because the clear discards previous color contents, transition the acquired image from `eUndefined` to `eColorAttachmentOptimal`. If a later effect needs preserved swapchain contents, track the per-image layout instead.

```cpp
void VulkanRenderer::BeginSwapchainRendering(vk::raii::CommandBuffer& commandBuffer)
{
    const vk::ImageMemoryBarrier2 toColor{
        .srcStageMask = vk::PipelineStageFlagBits2::eNone,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _swapchain.Image(_imageIndex),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &toColor
    });

    vk::ClearValue clear{};
    clear.color.float32 = std::array{ 0.1f, 0.1f, 0.1f, 1.0f };

    const vk::RenderingAttachmentInfo colorAttachment{
        .imageView = _swapchain.ImageView(_imageIndex),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear
    };
    const vk::RenderingInfo renderingInfo{
        .renderArea = { .offset = { 0, 0 }, .extent = _swapchain.Extent() },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    commandBuffer.beginRendering(renderingInfo);
    _renderingActive = true;
}
```

Call `BeginSwapchainRendering(frame.commandBuffer)` at the end of a successful `BeginFrame()`. Keep rendering active through `EndFrame()` because the current engine places ImGui between `EndFrame()` and `Present()`.

Finish rendering and transition to presentation inside `Present()`:

```cpp
void VulkanRenderer::EndSwapchainRendering(vk::raii::CommandBuffer& commandBuffer)
{
    commandBuffer.endRendering();
    _renderingActive = false;

    const vk::ImageMemoryBarrier2 toPresent{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eNone,
        .dstAccessMask = vk::AccessFlagBits2::eNone,
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = _swapchain.Image(_imageIndex),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &toPresent
    });
}
```

Submit with Synchronization 2, then present:

```cpp
void VulkanRenderer::Present()
{
    if (!_frameReady)
        return;

    try
    {
        VulkanFrameResources& frame = _frames[_frameIndex];
        EndSwapchainRendering(frame.commandBuffer);
        frame.commandBuffer.end();

        const vk::CommandBufferSubmitInfo commandInfo{
            .commandBuffer = *frame.commandBuffer
        };
        const vk::SemaphoreSubmitInfo waitInfo{
            .semaphore = *frame.imageAvailable,
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
        };
        const vk::SemaphoreSubmitInfo signalInfo{
            .semaphore = *frame.renderFinished,
            .stageMask = vk::PipelineStageFlagBits2::eAllGraphics
        };
        const vk::SubmitInfo2 submitInfo{
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo
        };
        _device.GraphicsQueue().submit2(submitInfo, *frame.inFlightFence);

        const vk::SwapchainKHR swapchainHandle = *_swapchain.Swapchain();
        const vk::Semaphore finished = *frame.renderFinished;
        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &finished,
            .swapchainCount = 1,
            .pSwapchains = &swapchainHandle,
            .pImageIndices = &_imageIndex
        };
        const vk::Result presentResult = _device.PresentQueue().presentKHR(presentInfo);

        _resizePending |= presentResult == vk::Result::eErrorOutOfDateKHR ||
                          presentResult == vk::Result::eSuboptimalKHR;
        if (presentResult != vk::Result::eSuccess &&
            presentResult != vk::Result::eSuboptimalKHR &&
            presentResult != vk::Result::eErrorOutOfDateKHR)
        {
            throw std::runtime_error("Vulkan presentation failed");
        }

        _frameReady = false;
        _frameIndex = (_frameIndex + 1) % FramesInFlight;
    }
    catch (const std::exception& error)
    {
        Debug::LogError("VulkanRenderer::Present: ", error.what());
        _fatalError = true;
        _frameReady = false;
    }
}
```

Do not advance `_frameIndex` if no frame was submitted.

Checkpoint: the Vulkan window must clear to dark gray for hundreds of frames, resize, minimize, restore, and close with validation clean.

## Stage 7: Add a Hardcoded Triangle and Pipeline

Do this before touching the asset registries. It isolates shader loading and pipeline state from the engine's current OpenGL-owned resource problem.

Add `VulkanShaderModule` and `VulkanPipeline`. Store owning handles in dependency order:

```cpp
class VulkanPipeline
{
public:
    bool Create(
        const VulkanDevice& device,
        vk::Format colorFormat,
        std::span<const uint32_t> vertexSpirv,
        std::span<const uint32_t> fragmentSpirv);
    void Shutdown() noexcept;

    vk::Pipeline Handle() const { return *_pipeline; }
    vk::PipelineLayout Layout() const { return *_layout; }

private:
    vk::raii::PipelineLayout _layout{ nullptr };
    vk::raii::Pipeline _pipeline{ nullptr };
};
```

Shader modules may be local variables inside `Create()` because a completed graphics pipeline does not need them afterward:

```cpp
vk::raii::ShaderModule vertexModule(
    device.Device(),
    vk::ShaderModuleCreateInfo{
        .codeSize = vertexSpirv.size_bytes(),
        .pCode = vertexSpirv.data()
    });
```

Use `vk::PipelineRenderingCreateInfo` in the pipeline `pNext` chain because this backend uses dynamic rendering:

```cpp
const vk::PipelineRenderingCreateInfo renderingInfo{
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &colorFormat
};

vk::GraphicsPipelineCreateInfo pipelineInfo{
    .pNext = &renderingInfo,
    // shader stages and fixed-function state omitted here
    .layout = *_layout,
    .renderPass = nullptr
};

_pipeline = vk::raii::Pipeline(
    device.Device(), nullptr, pipelineInfo);
```

Use dynamic viewport and scissor. In `EndFrame()`, while dynamic rendering is active, bind the pipeline, set viewport/scissor from `_swapchain.Extent()`, and call `draw(3, 1, 0, 0)`.

Compile a dedicated Vulkan triangle shader to SPIR-V during development; do not load the engine's existing GLSL directly into Vulkan. Keep this triangle independent of `DrawCmd` and delete it once the first engine mesh works.

Checkpoint: RenderDoc must show one dynamic-rendering pass, one graphics pipeline, and one three-vertex draw.

## Stage 8: Fix the Backend Resource Seam

This stage is required before `VulkanRenderer::Submit` can render existing assets. Today `Graphics::Mesh`, `Shader`, and `Texture2D` own OpenGL handles, and the registries construct those concrete types. A Vulkan renderer cannot recover upload data or SPIR-V from them.

Do not add Vulkan handles to those existing classes. Convert them into backend-neutral interfaces and move the current implementations under OpenGL-specific classes:

```cpp
class Mesh
{
public:
    virtual ~Mesh() = default;
    virtual RendererBackend Backend() const = 0;
    virtual bool IsValid() const = 0;
    virtual uint32_t VertexCount() const = 0;
    virtual uint32_t IndexCount() const = 0;
    virtual PrimitiveTopology Topology() const = 0;
};

class OpenGLMesh final : public Mesh
{
    // Move the current VertexArray/VertexBuffer/IndexBuffer implementation here.
};

class VulkanMesh final : public Mesh
{
    VulkanBuffer _vertexBuffer;
    VulkanBuffer _indexBuffer;
    VertexLayout _layout;
    uint32_t _vertexCount = 0;
    uint32_t _indexCount = 0;
    PrimitiveTopology _topology = PrimitiveTopology::TRIANGLES;
};
```

Apply the same pattern to `Shader` and `Texture2D`. `Material` can remain a backend-neutral value containing pointers to the abstract shader and texture plus `RenderState`.

Add a resource factory owned by each renderer:

```cpp
class IGraphicsResourceFactory
{
public:
    virtual ~IGraphicsResourceFactory() = default;

    virtual std::unique_ptr<Mesh> CreateMesh(
        const MeshUploadData& data,
        std::string_view label) = 0;
    virtual std::unique_ptr<Texture2D> CreateTextureRGBA(
        const unsigned char* pixels,
        int width,
        int height,
        std::string_view label) = 0;
    virtual std::unique_ptr<Shader> CreateShader(
        const ShaderAssetSource& source) = 0;
};
```

Expose it from `IRenderer`:

```cpp
virtual IGraphicsResourceFactory& ResourceFactory() = 0;
```

Bind the factory after the renderer initializes and before loading assets:

```cpp
if (!renderer->Init(window.handle))
    return false;

assets.SetGraphicsResourceFactory(&renderer->ResourceFactory());
```

Change registries from values to polymorphic owners:

```cpp
std::unordered_map<AssetId, std::unique_ptr<Graphics::Mesh>> _meshes;
```

`MeshRegistry::Create` retains its existing `MeshUploadData` construction, but delegates GPU creation:

```cpp
auto mesh = _factory->CreateMesh(meshUploadData, name);
if (!mesh || !mesh->IsValid())
    return {};

_meshes.emplace(handle.id, std::move(mesh));
```

The texture registry must decode image files into CPU RGBA pixels first, then call `CreateTextureRGBA`. The shader registry must pass a logical source description containing both OpenGL GLSL paths and Vulkan SPIR-V paths:

```cpp
struct ShaderAssetSource
{
    std::string vertexGlsl;
    std::string fragmentGlsl;
    std::string vertexSpirv;
    std::string fragmentSpirv;
};
```

Use a build step to compile `*.vert` and `*.frag` into the matching `*.spv` paths. Do not compile Vulkan GLSL at runtime.

At the renderer boundary, reject a resource from the wrong backend before casting:

```cpp
const auto* mesh = cmd.mesh;
if (!mesh || mesh->Backend() != RendererBackend::VULKAN)
{
    Debug::LogError("VulkanRenderer received a non-Vulkan mesh");
    return;
}

const auto& vulkanMesh = static_cast<const VulkanMesh&>(*mesh);
```

This check catches registry/factory wiring bugs without spreading Vulkan types outside `graphics/vulkan`.

## Stage 9: Implement Vulkan Buffers and Mesh Draws

`VulkanBuffer` groups memory and a buffer. Declare memory first so the buffer is destroyed before its bound memory:

```cpp
class VulkanBuffer
{
public:
    bool Create(
        const VulkanDevice& device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags memoryProperties);
    void Write(const void* source, vk::DeviceSize size, vk::DeviceSize offset = 0);
    void Shutdown() noexcept;

    vk::Buffer Handle() const { return *_buffer; }

private:
    const VulkanDevice* _device = nullptr;
    vk::DeviceSize _size = 0;
    vk::raii::DeviceMemory _memory{ nullptr };
    vk::raii::Buffer _buffer{ nullptr };
};
```

Creation order is buffer, requirements, memory allocation, then binding, even though declaration order is memory before buffer:

```cpp
_buffer = vk::raii::Buffer(device.Device(), vk::BufferCreateInfo{
    .size = size,
    .usage = usage,
    .sharingMode = vk::SharingMode::eExclusive
});

const vk::MemoryRequirements requirements = _buffer.getMemoryRequirements();
const uint32_t memoryType = FindMemoryType(
    device.PhysicalDevice(),
    requirements.memoryTypeBits,
    memoryProperties);

_memory = vk::raii::DeviceMemory(device.Device(), vk::MemoryAllocateInfo{
    .allocationSize = requirements.size,
    .memoryTypeIndex = memoryType
});
_buffer.bindMemory(*_memory, 0);
```

Use a host-visible staging buffer and device-local vertex/index buffers for the final path. A temporary host-visible mesh path is acceptable only as a bounded checkpoint; replace it before texture uploads.

Record indexed draws in `VulkanRenderer::EndFrame()`:

```cpp
const vk::Buffer vertexBuffer = vulkanMesh.VertexBuffer();
const vk::DeviceSize offset = 0;
commandBuffer.bindVertexBuffers(0, vertexBuffer, offset);
commandBuffer.bindIndexBuffer(
    vulkanMesh.IndexBuffer(), 0, vk::IndexType::eUint32);
commandBuffer.drawIndexed(vulkanMesh.IndexCount(), 1, 0, 0, 0);
```

Translate `VertexLayout` to `vk::VertexInputBindingDescription` and `vk::VertexInputAttributeDescription` when building the pipeline. Keep that conversion in `graphics/vulkan`; do not put Vulkan formats into `VertexLayout`.

Checkpoint order: indexed quad, `Primitive2D`, then imported OBJ. Compare each with OpenGL.

## Stage 10: Camera, Materials, Textures, and Depth

Keep the existing material semantics and map them inside the Vulkan backend:

```text
Set 0, binding 0: per-frame camera uniform buffer
Set 1, binding 0: material combined image sampler
Push constants: per-draw model matrix
```

Each `VulkanFrameResources` gets its own camera buffer and descriptor set. Never update one camera UBO while an earlier frame may still read it.

Convert the OpenGL projection only for Vulkan. A common correction is:

```cpp
glm::mat4 projection = camera.GetProjection();
projection[1][1] *= -1.0f;
// Generate or convert depth to Vulkan's 0..1 convention here.
```

Do not set global GLM Vulkan macros because the OpenGL backend must retain its current projection convention.

Build `VulkanPipeline` from shader stages, vertex layout, `RenderState`, color format, and depth format. Those values form the pipeline-cache key. Translate `BlendMode`, depth test/write, culling, and topology in one Vulkan-only conversion module.

`VulkanImage` should group memory, image, and view; declare memory before image and image before view so destruction runs view, image, memory. `VulkanTexture2D` adds a sampler. Upload through a staging buffer, transition `eUndefined -> eTransferDstOptimal -> eShaderReadOnlyOptimal`, and update the material descriptor.

Create one depth image per swapchain extent and include its attachment in `vk::RenderingInfo`. Recreate it with the swapchain and rebuild pipelines if an attachment format changes.

Checkpoint: camera transforms, indexed meshes, textures, blend modes, and depth must match the OpenGL reference closely enough to expose convention errors.

## Stage 11: Integrate Dear ImGui Without Breaking the Frame Boundary

The repository currently vendors the OpenGL and GLFW ImGui backends but not `imgui_impl_vulkan.*`. Add `imgui_impl_vulkan.cpp` and `.h` from the exact same Dear ImGui revision as the existing `1.92.7 WIP` files, then add them to the Visual Studio project. Mixing backend revisions can compile and still corrupt texture or viewport state.

Make the renderer own backend-specific ImGui calls. Extend `IRenderer` with:

```cpp
struct ImDrawData;

virtual bool InitImGui(GLFWwindow* window) = 0;
virtual void BeginImGuiFrame() = 0;
virtual void RenderImGui(ImDrawData* drawData) = 0;
virtual void ShutdownImGui() = 0;
```

`ImGuiLayer` still owns the ImGui context and UI construction, but delegates backend work:

```cpp
bool ImGuiLayer::Init(GLFWwindow* window, Graphics::IRenderer& renderer)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Configure docking and style here.
    _initialized = renderer.InitImGui(window);
    return _initialized;
}

void ImGuiLayer::Begin(Graphics::IRenderer& renderer)
{
    renderer.BeginImGuiFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End(Graphics::IRenderer& renderer)
{
    ImGui::Render();
    renderer.RenderImGui(ImGui::GetDrawData());
}
```

OpenGL moves its current `ImGui_ImplGlfw_*` and `ImGui_ImplOpenGL3_*` calls into these methods. Vulkan uses `ImGui_ImplGlfw_InitForVulkan`, initializes `ImGui_ImplVulkan` with raw handles unwrapped from the RAII owners, enables dynamic rendering with the swapchain color format, and records:

```cpp
ImGui_ImplVulkan_RenderDrawData(
    drawData,
    static_cast<VkCommandBuffer>(*_frames[_frameIndex].commandBuffer));
```

Call this after `VulkanRenderer::EndFrame()` records world draws and before `VulkanRenderer::Present()` ends dynamic rendering. Disable ImGui multi-viewports for the first Vulkan integration; add Vulkan platform-window swapchains only after the main viewport is stable.

The exact `ImGui_ImplVulkan_InitInfo` fields are revision-sensitive, so use the header vendored with this project rather than copying a struct initializer from another ImGui version.

## Stage 12: Correct Engine Initialization and Shutdown Order

Initialization must be:

```text
window
renderer context/device/swapchain
asset resource factory
ImGui backend
GPU assets
```

Shutdown must reverse the GPU dependencies. The current engine shuts the renderer down before assets; that would destroy the Vulkan device while asset-owned buffers and images still exist. Change it to:

```cpp
void Engine::Shutdown()
{
    Debug::CLog("========== Shutting down engine... =========\n");

    if (renderer)
    {
        renderer->WaitIdle();       // Add to IRenderer or perform inside the next calls.
        assets.Clear();             // Meshes, shaders, textures, materials.
        imgui.Shutdown(*renderer);  // Descriptor pool and ImGui Vulkan objects.
        renderer->Shutdown();       // Frames, swapchain, device, context.
        renderer.reset();
    }

    window.Shutdown();
    Debug::CLog("Engine shutdown complete\n");
}
```

If `WaitIdle()` is not added to `IRenderer`, make `VulkanRenderer::Shutdown()` wait before any child owner is cleared, but assets still must be cleared before that method destroys the device. A small backend-neutral `IRenderer::WaitIdle()` is the clearer contract.

`VulkanRenderer::Shutdown()` must clear in this order:

```text
wait idle
ImGui Vulkan backend if still active
pipelines, descriptors, textures, buffers, depth image
frame resources
swapchain image views and swapchain
logical device and queues
surface, debug messenger, instance
window pointer and state flags
```

Every shutdown method must tolerate partial initialization and repeated calls.

## Stage 13: Verification Gates

Do not implement several gates at once. A failure is much easier to isolate when each gate was previously clean.

1. OpenGL builds and runs after the C++20/project-file change.
2. Vulkan instance, validation messenger, and surface open and close.
3. Physical device and separate graphics/present queue discovery work.
4. Logical device and swapchain create and destroy.
5. Resize, minimize, restore, and shutdown work before rendering.
6. Two frames acquire, clear, submit, and present for at least several minutes.
7. Hardcoded triangle renders through dynamic rendering.
8. Backend resource factory creates OpenGL resources without changing OpenGL output.
9. Vulkan indexed quad renders through `DrawCmd`.
10. Imported OBJ, camera, material, texture, and depth work.
11. ImGui main viewport renders after world draws.
12. Debug and Release both run with Vulkan selected.

At every Vulkan gate:

- Run with `VK_LAYER_KHRONOS_validation` enabled.
- Treat validation errors as implementation failures, not warnings to defer.
- Capture one frame in RenderDoc after the first clear, triangle, indexed mesh, and textured mesh milestones.
- Test closing immediately after startup, because partial-frame shutdown exposes lifetime bugs.
- Test repeated minimize/restore and rapid resize.

## Common Refactor Failures

If a RAII child crashes during destruction, its parent was probably destroyed first. Check member declaration order and explicit shutdown order before blaming Vulkan-Hpp.

If acquire throws on resize instead of returning `eErrorOutOfDateKHR`, `VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS` was not defined before the first Vulkan-Hpp include in that translation unit.

If designated initializers fail, the project is not compiling as C++20 or `VULKAN_HPP_NO_STRUCT_CONSTRUCTORS` was defined inconsistently.

If GLFW surface creation succeeds and shutdown double-frees, both a raw cleanup path and `vk::raii::SurfaceKHR` own the same `VkSurfaceKHR`. Delete the raw destroy call.

If Vulkan receives an OpenGL mesh, the asset registry was populated before the renderer's resource factory was installed or a registry still stores concrete OpenGL values.

If resizing hangs, verify that the frame fence is reset only after successful image acquisition and that zero framebuffer extents postpone recreation.

If the clear works but presentation validation fails, check the acquire semaphore wait stage, the color-to-present barrier, and the render-finished semaphore passed to present.

If ImGui disappears or validation reports an active rendering error, ensure its draw data is recorded after world draws while dynamic rendering is still active, and only then let `Present()` call `endRendering()`.
