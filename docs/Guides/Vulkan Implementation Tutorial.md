# Vulkan Implementation Tutorial

This is the implementation guide for adding Vulkan alongside the existing OpenGL backend. It follows the engine's current boundaries instead of building a standalone Vulkan sample.

The non-negotiable boundary is:

```text
Scene / future ECS / manual test
    -> Rendering::RenderItem
    -> RenderResourceResolver
    -> Graphics::DrawCmd
    -> Graphics::IRenderer
       -> OpenGLRenderer
       -> VulkanRenderer
```

`DrawCmd` remains backend-neutral. Vulkan handles stay in `GameEngine/src/graphics/vulkan/`. Do not put `Vk*` handles in Scene, assets, importers, render items, or `DrawCmd`.

## Current State

The engine already has `IRenderer`, `OpenGLRenderer`, `VulkanRenderer`, `VulkanContext`, a separate `Present()` phase, Vulkan SDK project configuration, a GLFW no-API window for Vulkan, and a Vulkan 1.3 instance, debug messenger, and window surface.

The next class is `VulkanDevice`. Complete each chapter and run its verification before continuing. Keep OpenGL as the default backend until the corresponding Vulkan frame path is usable.

## Chapters

1. [[Vulkan/00 - Overview and Setup|Overview and Setup]]
2. [[Vulkan/01 - Context and Validation|Context and Validation]]
3. [[Vulkan/02 - Physical Device and Queues|Physical Device and Queues]]
4. [[Vulkan/03 - Logical Device and Swapchain|Logical Device and Swapchain]]
5. [[Vulkan/04 - Frames and Synchronization|Frames and Synchronization]]
6. [[Vulkan/05 - Clear Frame and Dynamic Rendering|Clear Frame and Dynamic Rendering]]
7. [[Vulkan/06 - Triangle and Pipeline|Triangle and Pipeline]]
8. [[Vulkan/07 - Buffers and Meshes|Buffers and Meshes]]
9. [[Vulkan/08 - Descriptors Camera and Materials|Descriptors, Camera, and Materials]]
10. [[Vulkan/09 - Textures Depth and ImGui|Textures, Depth, and ImGui]]

## Required Rules

- Target Vulkan 1.3. Use dynamic rendering and Synchronization 2 from the beginning.
- Check every `VkResult`; never continue after a failed Vulkan creation call.
- Initialize handles to `VK_NULL_HANDLE`, delete copy operations for owning classes, and make shutdown safe after partial initialization.
- Destroy dependent objects before their parents: frame resources and swapchain before device, then surface before instance.
- Use validation in Debug. Fix the first validation error rather than hiding it.
- Use GLFW framebuffer dimensions, not logical window dimensions, for swapchain extent.

Related design: [[Architecture/Vulkan Backend]]
