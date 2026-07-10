# Vulkan Backend

## Status

This is the engine's active implementation focus until the Vulkan backend roadmap is complete.

OpenGL remains the working reference backend during the transition. The target is two selectable backends that share render submission, asset import data, and backend-neutral graphics contracts. Only one backend is active for an engine instance.

## Target API

Require Vulkan 1.3 with:

- Dynamic rendering through `vkCmdBeginRendering` and `vkCmdEndRendering`.
- Synchronization 2 through `vkCmdPipelineBarrier2` and `vkQueueSubmit2`.
- `VK_KHR_swapchain` for presentation.
- `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils` in development builds.

Do not add a Vulkan 1.2 or legacy render-pass fallback path.

## Architecture

```text
ManualRenderTest / future Scene / ECS / E-CS
    -> Rendering::RenderItem and RenderResourceResolver
    -> Graphics::DrawCmd
    -> Graphics::IRenderer
       -> OpenGLRenderer
       -> VulkanRenderer
```

`Graphics::Renderer` becomes a backend-neutral renderer contract. `OpenGLRenderer` and `VulkanRenderer` are backend implementations.

`DrawCmd` remains resolved render data. It must not contain asset-manager references, asset handles, Scene data, GameObject data, or ECS data.

## Folder Direction

```text
GameEngine/src/
  graphics/                  # Backend-neutral contracts and value types
  graphics/opengl/           # Existing OpenGL implementation after migration
  graphics/vulkan/           # Vulkan implementation
  rendering/                 # RenderItem and handle-to-resource resolution
  assets/                    # Handles, imported CPU data, registries
```

The initial Vulkan-specific files should be:

- `vulkan_context.*`: instance, debug messenger, window surface.
- `vulkan_device.*`: physical/logical device, queues, feature checks, allocator.
- `vulkan_swapchain.*`: swapchain images, views, format, extent, recreation.
- `vulkan_frame_resources.*`: command pool/buffer, fence, acquire/render semaphores, per-frame allocations.
- `vulkan_buffer.*`: `VkBuffer` and allocation.
- `vulkan_image.*`: `VkImage`, image view, allocation, layout helpers.
- `vulkan_texture2d.*`: uploaded image, view, sampler.
- `vulkan_shader_module.*`: SPIR-V shader stages.
- `vulkan_pipeline.*`: pipeline layouts, graphics pipelines, pipeline cache.
- `vulkan_descriptors.*`: layouts, pools, descriptor allocation and updates.
- `vulkan_renderer.*`: frame lifecycle and `DrawCmd` recording.
- `vulkan_utils.*`: Vulkan-result checking, debug names, and enum conversion helpers.

## Backend-Neutral Contracts

Keep these independent of OpenGL and Vulkan:

- `DrawCmd`
- `MeshUploadData`
- `VertexLayout`
- `PrimitiveTopology`
- `RenderState` and `BlendMode`
- Camera world data
- Asset handles and imported mesh/model data
- `RenderItem` and `RenderResourceResolver`

GPU resource creation must move behind a backend resource factory. Registries should eventually store backend-neutral resource ownership, such as `std::unique_ptr<Graphics::Mesh>`, rather than concrete OpenGL values.

Do not make `VertexArray` backend-neutral. It is an OpenGL-only implementation detail. Vulkan meshes use vertex and index buffers plus pipeline vertex-input descriptions.

## Resource Mapping

| Existing concept | Vulkan direction |
|---|---|
| `Renderer` | `VulkanRenderer` records and submits command buffers. |
| `Mesh` | Device-local vertex/index buffers and draw metadata. |
| `VertexLayout` | Vulkan vertex binding and attribute descriptions. |
| `Shader` | SPIR-V shader modules; modules feed pipeline creation. |
| `Material` | Pipeline, descriptor bindings, render state, parameter metadata. |
| `Texture2D` | Image, image view, sampler, shader-read layout. |
| `Camera2D` | Per-frame camera uniform buffer. |
| `DrawCmd::transform` | Per-draw push constant in the first Vulkan path. |

A Vulkan graphics pipeline is the equivalent of the current shader program plus fixed render state. Its cache key must include shader stages, vertex layout, render state, and active color/depth attachment formats.

## Frame Lifecycle

```text
Wait for current frame fence
-> acquire swapchain image
-> reset current command pool
-> record image transition to color attachment
-> begin dynamic rendering
-> record DrawCmd objects
-> end dynamic rendering
-> record image transition to present
-> submit with Synchronization 2
-> present
-> advance frame index
```

Use two frames in flight initially. A resize callback only records a resize request. Recreate the swapchain at a safe frame boundary, including its image views and depth image. Rebuild format-dependent pipelines when the swapchain format changes.

## Shader and Descriptor Convention

Vulkan shaders are compiled offline from GLSL to SPIR-V. Do not load raw GLSL source into Vulkan at runtime.

Start with this binding convention:

```text
Set 0, binding 0: per-frame camera uniform buffer
Set 1, binding 0: material combined image sampler
Push constants: per-draw model transform
```

Later material parameters can use named/semantic asset properties that resolve to descriptor bindings, push constants, or uniform-buffer offsets. Per-object values must not mutate shared material descriptors.

## Camera Note

The current camera projection is OpenGL-oriented. Vulkan uses a 0..1 depth range, and framebuffer Y orientation must be handled deliberately. Do not apply a global GLM configuration that silently changes the working OpenGL backend. Make the projection conversion backend-specific or make projection generation explicitly backend-aware.

## Completion Definition

The Vulkan track is complete when:

- Vulkan and OpenGL are selectable at initialization.
- Vulkan validation is clean for the exercised render path.
- The Vulkan backend handles resize, minimize, shutdown, and device-loss errors predictably.
- Existing indexed meshes and asset-loaded textures render through the existing submission boundary.
- Materials bind shaders, textures, render state, and basic camera/object data.
- ImGui renders with the Vulkan backend.
- No Scene, GameObject, ECS, or importer code depends on Vulkan types.
