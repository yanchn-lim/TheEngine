# Vulkan Backend

`VulkanGraphicsDevice` implements `IGraphicsDevice` and `IGraphicsCommandList`. It reuses `VulkanContext`, `VulkanDevice`, `VulkanSwapchain`, and per-frame RAII owners.

The device owns Vulkan buffers, images, views, samplers, shader modules, descriptor objects, pipeline layouts, and graphics pipelines behind typed handles. Texture upload uses a staging buffer and explicit layout transitions. Frame recording uses synchronization 2 and dynamic rendering.

The frame sequence is acquire, record, submit, and present. Resize marks the swapchain for recreation. A zero-size framebuffer skips work until the window has a valid extent.

The swapchain prefers an `UNORM` surface format. The current shaders, decoded textures, ImGui colors, and OpenGL default framebuffer use display-ready values, so an sRGB swapchain would encode the output again and wash out the image. Convert both back ends together before a future move to a linear-light workflow.

`VulkanImGuiBackend` initializes the GLFW and Vulkan ImGui integrations. It records UI draw data in the active dynamic-rendering pass before `Renderer::EndFrame()` submits the command buffer. Vulkan multi-viewport support stays disabled because each platform viewport needs separate Vulkan swapchain ownership.

The vendored Vulkan ImGui backend comes from the official Dear ImGui repository at commit `0db591935f08c73f1e0726869a92ca803e8660a9`. Update it with the other vendored ImGui files when the ImGui revision changes.

Vulkan code does not resolve assets or read ECS data. It receives the same device descriptions and command calls as OpenGL.
