# Graphics Backend

The public graphics boundary is `Graphics::IGraphicsDevice` and `Graphics::IGraphicsCommandList`. OpenGL and Vulkan implement the same resource, frame, presentation, and command operations.

The dependency direction is:

```text
Editor or user code
  -> ECS component or RenderWorld
  -> Rendering::Renderer
  -> Rendering::RenderResourceManager
  -> Graphics::IGraphicsDevice
       -> OpenGLGraphicsDevice
       -> VulkanGraphicsDevice
```

Backend-neutral headers contain typed handles and project enums. They do not contain OpenGL or Vulkan types. Native objects stay under `graphics/opengl/` or `graphics/vulkan/`.

OpenGL executes command-list operations immediately. Vulkan records them in the active command buffer. Both paths use the same asset records, render items, resolved draws, frame constants, and draw constants.
