# Overview and Setup

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

Vulkan replaces OpenGL's implicit context and global state with explicit objects and synchronization. The engine owns those objects in C++ classes, but uses the normal Vulkan C API because it maps directly to the official specification, validation output, RenderDoc, and most reference material.

```text
VkInstance
    -> VkSurfaceKHR
        -> VkPhysicalDevice
            -> VkDevice and queues
                -> VkSwapchainKHR
                    -> frame resources, buffers, images, pipelines
```

Create objects from top to bottom. Destroy them in the reverse order.

## Existing Engine Boundary

`VulkanRenderer` implements `Graphics::IRenderer`. It receives finished `DrawCmd` values exactly as `OpenGLRenderer` does. It is responsible for resolving backend resources, recording Vulkan commands, submitting work, and presenting. Asset registries and Scene code do not know which backend is active.

The lifecycle stays:

```text
BeginFrame()
Submit(drawCmd) ...
EndFrame()
ImGui overlay
Present()
```

`EndFrame()` records the game world. `Present()` completes the backend's presentation work. Keep them separate because ImGui must be recorded after world rendering and before presentation.

## SDK and Project Setup

Install the LunarG Vulkan SDK and ensure the machine-level `VULKAN_SDK` environment variable is set. The Visual Studio x64 Debug and Release configurations need:

```text
Additional Include Directories: $(VULKAN_SDK)\Include
Additional Library Directories: $(VULKAN_SDK)\Lib
Additional Dependencies: vulkan-1.lib
```

Restart Visual Studio after changing environment variables. A path that expands to `\Lib` normally means Visual Studio did not inherit `VULKAN_SDK`.

## Files You Will Create

Keep backend-neutral code under `graphics/` and Vulkan implementation under `graphics/vulkan/`:

```text
graphics/vulkan/
  vulkan_context.*
  vulkan_device.*
  vulkan_swapchain.*
  vulkan_frame_resources.*
  vulkan_buffer.*
  vulkan_image.*
  vulkan_texture2d.*
  vulkan_shader_module.*
  vulkan_pipeline.*
  vulkan_descriptors.*
  vulkan_renderer.*
```

Add every file to both `GameEngine.vcxproj` and the corresponding `graphics/vulkan` Visual Studio filter. The compiler only sees files listed by the project.

## Result Checking

Add one small helper before additional Vulkan work. Adapt it to the existing engine logging style:

```cpp
inline void VkCheck(VkResult result, const char* operation)
{
    if (result == VK_SUCCESS)
    {
        return;
    }

    ENGINE_ERROR("Vulkan failed: {} ({})", operation, static_cast<int>(result));
    ENGINE_ASSERT(false, "Vulkan operation failed");
}

#define VK_CHECK(expression) VkCheck((expression), #expression)
```

For initialization, return `false` after logging and let the owner call `Shutdown()`. `VK_CHECK` is appropriate once a frame is already in a valid state and failure cannot be recovered locally. Do not write a helper that silently discards failed results.

## Before Continuing

Build the current Vulkan skeleton and select `RendererBackend::VULKAN` temporarily. A blank GLFW no-API window should open and close without validation errors. Restore OpenGL as the default afterwards.

You are ready for [[01 - Context and Validation]].
