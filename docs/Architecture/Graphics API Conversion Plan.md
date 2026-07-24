# Graphics API Conversion Plan

## 1. Purpose

Use this plan to replace the current graphics API with a user-first graphics API.

The new API must support these operations:

- User code creates a render instance without an entity.
- User code submits a render item for one frame.
- OpenGL and Vulkan render the same render data.

Do the steps in the specified sequence. Complete each check before you start the next step.

This document uses controlled language based on ASD-STE100 Issue 9. API names are project technical nouns.

## 2. Technical Terms

Use each term only with the meaning in this section.

- **Asset handle**: An identifier for a mesh, texture, shader, material, or model asset.
- **GPU handle**: An identifier for a resource that the graphics device owns.
- **Render instance**: A persistent object in the render world.
- **Render instance handle**: An identifier for a render instance.
- **Render item**: A complete request to render one object.
- **Transient render item**: A render item that is valid for one frame.
- **Render world**: The owner of persistent render instances and transient render items.
- **Render queue**: A frame-local list of visible render items.
- **Draw packet**: Backend-neutral data for one graphics draw operation.
- **Renderer**: The system that converts render-world data into draw packets.
- **Graphics device**: The low-level interface for GPU resources and GPU commands.
- **Backend**: An OpenGL or Vulkan implementation of the graphics device.
- **Render resource manager**: The system that converts asset handles into GPU handles.
- **Render view**: Camera, viewport, layer, and render-target data for one view.

Do not use `renderer`, `graphics device`, and `backend` as synonyms.

## 3. Current Design

The current design has these conditions:

- `Graphics::IRenderer` owns the frame API and accepts `Graphics::DrawCmd`.
- `Graphics::DrawCmd` contains raw mesh and material pointers.
- `Graphics::Mesh` owns an OpenGL vertex array and OpenGL buffers.
- `Graphics::Mesh::Draw()` sends an OpenGL draw operation.
- `Graphics::Shader` owns an OpenGL program and sets OpenGL uniforms.
- `Graphics::Texture2D` loads image files and owns an OpenGL texture.
- `Graphics::Material` contains raw shader and texture pointers.
- Asset registries construct and own OpenGL resources.
- `RenderResourceResolver` converts asset handles into raw graphics pointers.
- `OpenGLRenderer` resolves state and sends draw operations.
- `VulkanRenderer` clears and presents the swapchain.
- `VulkanRenderer::EndFrame()` does not draw the submitted commands.
- `ImGuiLayer` calls the OpenGL ImGui backend directly.
- `Window::Init()` initializes OpenGL functions.
- The repository does not contain the final editor implementation.
- The Visual Studio project uses C++20.

The conversion must remove each backend dependency from the user API.

## 4. Target Design

Use this dependency direction:

```text
User code
  -> RenderWorld

RenderWorld
  -> Renderer
  -> RenderResourceManager
  -> Graphics::IGraphicsDevice
       -> OpenGLGraphicsDevice
       -> VulkanGraphicsDevice
```

The editor and user code must use the same render-instance description.

```cpp
Rendering::MeshInstanceDesc description{
    .mesh = mesh,
    .material = material,
    .transform = transform,
    .visible = true
};
```

User code can create a persistent object without an entity.

```cpp
Rendering::RenderInstanceHandle instance =
    renderWorld.CreateMeshInstance(description);
```

User code can also submit the same description for one frame.

```cpp
renderWorld.DrawMeshOnce(description);
```

The persistent path and the transient path must create the same internal `RenderItem` type.

## 5. Conversion Sequence

### Step 1: Record the Baseline

1. Build the Debug x64 configuration with OpenGL.
2. Build the Release x64 configuration with OpenGL.
3. Run the manual model test with OpenGL.
4. Record the image, frame statistics, and memory statistics.
5. Run the Vulkan clear-frame path.
6. Enable Vulkan validation.
7. Record all validation errors.
8. Add a short baseline report to `docs/Architecture/`.

Do not change graphics behavior in this step.

The step is complete when both backends have a recorded baseline.

### Step 2: Freeze the Public Requirements

1. Create `docs/Architecture/Graphics API.md`.
2. Add the persistent mesh-instance example from Section 4.
3. Add the persistent sprite-instance example.
4. Add the one-frame mesh example.
5. Add the resource creation example for a procedural mesh.
6. Add the resource update example for a dynamic mesh.
7. Define the owner of each returned handle.
8. Define the valid call time for each operation.
9. Define the error result for each operation.
10. Define the thread rule for each operation.

Use these initial rules:

- `RenderWorld` owns render instances.
- The caller owns asset handles.
- The graphics device owns GPU resources.
- Persistent handles remain valid until destruction or world shutdown.
- Transient render items remain valid for one frame.
- The API copies transient render-item data during submission.
- Only the render thread calls the graphics device.
- User code does not call the graphics device.

The step is complete when the examples need no backend term.

### Step 3: Set the Source Layout

Create these source areas:

```text
src/graphics/
  graphics_device.hpp
  graphics_device_factory.hpp
  graphics_handles.hpp
  graphics_descriptions.hpp
  graphics_command_list.hpp
  graphics_capabilities.hpp
  opengl/
  vulkan/

src/rendering/
  renderer.hpp
  render_world.hpp
  render_instance.hpp
  render_item.hpp
  render_queue.hpp
  render_view.hpp
  render_resource_manager.hpp
  render_sort.hpp

src/components/
  mesh_renderer_component.hpp
  sprite_renderer_component.hpp

src/systems/
  render_system.hpp
```

Move no implementation in this step.

Add empty files only when the next step uses them. Add each file to the Visual Studio project and filter.

Change both project configurations to C++20. Build OpenGL after this change.

The step is complete when the existing program builds with C++20.

### Step 4: Add Typed Handles

1. Add typed GPU handles in `graphics_handles.hpp`.
2. Add `GpuBufferHandle`.
3. Add `GpuTextureHandle`.
4. Add `GpuSamplerHandle`.
5. Add `GpuShaderHandle`.
6. Add `GpuPipelineHandle`.
7. Add `GpuRenderTargetHandle`.
8. Add `RenderInstanceHandle` in the rendering layer.

Use an index and a generation in each runtime handle.

```cpp
struct GpuTextureHandle
{
    uint32_t index = 0;
    uint32_t generation = 0;
};
```

Reserve the zero value for an invalid handle. Do not use one handle type in place of another handle type.

Add these tests:

- A default handle is invalid.
- A live handle is valid.
- Destruction invalidates the old generation.
- Slot reuse does not make an old handle valid.
- A handle from one device is not valid in another device.

Do not change `Assets::MeshHandle` or the other asset handles in this step.

The step is complete when all handle tests pass.

### Step 5: Add Backend-Neutral Descriptions

Add plain data types for GPU resource creation.

Add these descriptions:

- `BufferDesc`
- `TextureDesc`
- `SamplerDesc`
- `ShaderModuleDesc`
- `VertexLayoutDesc`
- `GraphicsPipelineDesc`
- `RenderPassDesc`
- `ViewportDesc`
- `ScissorDesc`

Add these enums:

- Buffer usage
- Memory usage
- Texture format
- Texture usage
- Texture filter
- Texture address mode
- Shader stage
- Index format
- Primitive topology
- Cull mode
- Front-face mode
- Compare operation
- Blend factor
- Blend operation
- Load operation
- Store operation

Use project enums in public headers. Do not use `GLenum`, `VkFormat`, or other backend types.

Keep `VertexLayout` as backend-neutral data. Remove the OpenGL statement from its component-count function.

Add an explicit resource label to each resource description. Use the label in diagnostics and memory statistics.

The step is complete when no description header includes OpenGL or Vulkan.

### Step 6: Define the Graphics Device

Add `Graphics::IGraphicsDevice`.

The interface must contain these groups:

- Device initialization and shutdown
- Capability queries
- GPU resource creation and destruction
- Frame start and frame end
- Resize notification
- Command-list access
- Presentation
- Idle wait for controlled shutdown

Use an interface similar to this shape:

```cpp
class IGraphicsDevice
{
public:
    virtual bool Initialize(const GraphicsDeviceDesc&) = 0;
    virtual const GraphicsCapabilities& GetCapabilities() const = 0;

    virtual GpuBufferHandle CreateBuffer(const BufferDesc&) = 0;
    virtual GpuTextureHandle CreateTexture(const TextureDesc&) = 0;
    virtual GpuSamplerHandle CreateSampler(const SamplerDesc&) = 0;
    virtual GpuShaderHandle CreateShader(const ShaderModuleDesc&) = 0;
    virtual GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc&) = 0;

    virtual void DestroyBuffer(GpuBufferHandle) = 0;
    virtual void DestroyTexture(GpuTextureHandle) = 0;
    virtual void DestroySampler(GpuSamplerHandle) = 0;
    virtual void DestroyShader(GpuShaderHandle) = 0;
    virtual void DestroyPipeline(GpuPipelineHandle) = 0;

    virtual FrameStatus BeginFrame(FrameContext&) = 0;
    virtual IGraphicsCommandList& GetCommandList(FrameContext&) = 0;
    virtual FrameStatus EndFrame(FrameContext&) = 0;
    virtual FrameStatus Present(FrameContext&) = 0;

    virtual void OnResize(uint32_t width, uint32_t height) = 0;
    virtual void WaitIdle() = 0;
    virtual void Shutdown() = 0;
};
```

Define `FrameStatus` values for success, skip, resize, device loss, and fatal failure.

Do not return backend handles from this interface.

Add `GraphicsDeviceFactory`. Let it create one backend from `RendererBackend`.

The step is complete when a test device can implement the full interface.

### Step 7: Define the Graphics Command List

Add `Graphics::IGraphicsCommandList`.

Include these initial operations:

- Start a render pass.
- End a render pass.
- Set a graphics pipeline.
- Set a viewport.
- Set a scissor rectangle.
- Set a vertex buffer.
- Set an index buffer.
- Set frame constants.
- Set material resources.
- Set draw constants.
- Draw vertices.
- Draw indexed vertices.
- Add a debug marker.

Do not expose OpenGL binding operations. Do not expose Vulkan barrier structures.

Make resource states an internal device responsibility for the first implementation. Add explicit state transitions only when a renderer feature needs them.

Add validation to the test command list. Reject these errors:

- A draw operation outside a render pass
- A draw operation without a pipeline
- An indexed draw operation without an index buffer
- A resource handle from a different device
- Two active render passes in one command list

The step is complete when command-list tests pass.

### Step 8: Convert the OpenGL Backend to a Graphics Device

1. Add `OpenGLGraphicsDevice` under `src/graphics/opengl/`.
2. Move OpenGL buffer ownership into the OpenGL backend.
3. Move OpenGL texture ownership into the OpenGL backend.
4. Move OpenGL shader-program ownership into the OpenGL backend.
5. Move vertex-array ownership into the OpenGL backend.
6. Move render-state conversion into the OpenGL backend.
7. Implement the graphics command list with OpenGL calls.
8. Implement the new GPU handle tables.
9. Keep memory labels and memory estimates.

The OpenGL backend can execute commands when it receives them. It does not have to store a native command buffer.

Move GLAD initialization from `Window::Init()` to `OpenGLGraphicsDevice::Initialize()`.

Move `glfwSwapBuffers()` to `OpenGLGraphicsDevice::Present()`.

Move `glViewport()` handling to the OpenGL command list or resize path.

Keep the old `OpenGLRenderer` as a temporary adapter. Let it call the new graphics device.

Do not change the manual test in this step.

The step is complete when the old manual test renders through `OpenGLGraphicsDevice`.

### Step 9: Separate Asset Data from GPU Data

Change each asset registry so that it owns asset data, not backend objects.

Add these asset records:

- `MeshAsset`
- `TextureAsset`
- `ShaderAsset`
- `MaterialAsset`

Use these initial contents:

```text
MeshAsset
  vertex data
  index data
  vertex layout
  primitive topology
  bounds
  submesh ranges

TextureAsset
  decoded pixels
  width
  height
  format
  sampler description

ShaderAsset
  stage variants
  entry points
  parameter metadata

MaterialAsset
  shader asset handle
  texture asset handles
  render state
  parameter values
```

Keep `ModelAsset` as a list of mesh asset handles. Keep material-slot data with the model or mesh asset.

Do not convert a model asset into one GPU mesh. Preserve its mesh and submesh limits.

Move `stb_image` use from `Graphics::Texture2D` to the asset import path.

Keep OBJ and primitive import data in the asset layer. Calculate mesh bounds during import.

Change material records to store asset handles. Remove raw shader and texture pointers.

Do not upload a GPU resource from an asset registry.

Keep asset IDs stable during asset reload. Define a separate version value for changed asset data.

The step is complete when asset tests run without a graphics device.

### Step 10: Add the Render Resource Manager

Add `Rendering::RenderResourceManager`.

Give it references to `AssetManager` and `IGraphicsDevice`.

Use it for these operations:

- Resolve a mesh asset handle to GPU buffers.
- Resolve a texture asset handle to a GPU texture and sampler.
- Resolve a shader asset handle to GPU shader modules.
- Resolve a material asset handle to material GPU data.
- Reuse a GPU resource for repeated asset resolution.
- Replace a GPU resource after asset reload.
- Destroy all GPU resources before device shutdown.

Use a resource state for each cache entry:

```text
Unloaded
Loading
Ready
Failed
```

Use synchronous upload for the first conversion. Keep the state model for later asynchronous upload.

Create fallback mesh, texture, shader, and material resources in this manager. Do not create fallbacks in asset lookup functions.

Replace `RenderResourceResolver` with this manager after the OpenGL path uses it.

The step is complete when asset handles resolve to typed GPU handles.

### Step 11: Define the Shader and Material Contract

Use one shader package for each logical shader.

Provide these variants at first:

- OpenGL GLSL source
- Vulkan SPIR-V bytecode

Add an offline shader build command for Vulkan SPIR-V. Add the command to the Visual Studio build or an asset build tool.

Use one binding convention:

```text
Set 0: frame and view data
Set 1: material data and textures
Draw constants: model transform and object data
```

Use names only in material and shader asset metadata. Convert names to binding locations during shader load.

Do not call `SetMat4()` or `SetInt()` during a draw operation.

Define a pipeline key with these values:

- Shader stages
- Vertex layout
- Primitive topology
- Render state
- Color format
- Depth format
- Sample count

Cache graphics pipelines in the render resource manager or graphics device. Use one owner only.

For the first version, let the render resource manager own the logical pipeline cache. Let the device own native pipeline objects.

The step is complete when the OpenGL backend renders without `Graphics::Shader::Bind()`.

### Step 12: Define Render Instances and Render Items

Add these persistent descriptions:

- `MeshInstanceDesc`
- `SpriteInstanceDesc`

Use these common fields:

- Asset handles
- Transform
- Visibility
- Render layer mask
- Shadow flags
- Sorting order
- Optional object ID
- Optional entity ID
- Bounds override
- Material overrides for submeshes

Add a generation table for render instances. Store the instance type in the table.

Add one internal `RenderItem` variant:

```cpp
using RenderItem = std::variant<MeshRenderItem, SpriteRenderItem>;
```

Keep asset handles in render instances. Do not store raw asset pointers or GPU pointers.

The step is complete when tests can create, update, and destroy render instances.

### Step 13: Implement the Render World

Add `Rendering::RenderWorld`.

Include these persistent operations:

- `CreateMeshInstance()`
- `CreateSpriteInstance()`
- `SetTransform()`
- `SetVisible()`
- `SetMesh()`
- `SetMaterial()`
- `SetLayers()`
- `UpdateMeshInstance()`
- `UpdateSpriteInstance()`
- `Destroy()`

Use the full update operations when multiple fields must change together. Do not expose an incomplete instance during one update.

Include these transient operations:

- `DrawMeshOnce()`
- `DrawSpriteOnce()`

Use the same description types for persistent and transient operations.

Copy transient descriptions into frame-owned storage. Clear this storage after frame execution.

Queue persistent changes if they come from a non-render thread. Apply the changes at the frame boundary.

Do not let a caller keep a pointer to render-world storage.

Add tests for these conditions:

- Create and destroy in one frame.
- Update after destruction.
- Double destruction.
- A stale handle update.
- A transient item after frame completion.
- A persistent instance without an entity.
- An entity ID that becomes invalid.

The step is complete when the render world passes all lifetime tests.

### Step 14: Add Render Views and the Render Queue

Add `RenderView` with these fields:

- View matrix
- Projection matrix
- Camera position
- Viewport
- Scissor rectangle
- Layer mask
- Color target
- Depth target
- Clear values
- View ID

Define one engine clip-space convention. Use a depth range from zero to one.

Apply backend correction in one documented location. Do not apply correction in asset data or user transforms.

Add `RenderQueue` as frame-local storage.

For each view, do these operations:

1. Read the persistent render instances.
2. Read the transient render items.
3. Reject invisible items.
4. Reject items outside the view layer mask.
5. Apply frustum culling when bounds are available.
6. Resolve asset handles.
7. Create sort keys.
8. Sort opaque items from front to back.
9. Sort transparent items from back to front.
10. Group compatible sprite items.
11. Create draw packets.

Use stable sorting when two items have the same key.

The step is complete when queue tests give deterministic output.

### Step 15: Add the Shared Renderer

Add one backend-neutral `Rendering::Renderer`.

Give it these owners or references:

- `RenderWorld`
- `RenderResourceManager`
- `IGraphicsDevice`
- Frame storage
- Render views
- Render queues

Use this frame sequence:

```text
Apply render-world changes
Start the graphics frame
Build render views
Build render queues
Record world passes
Record editor and UI passes
End the graphics frame
Present the frame
Clear transient data
```

The renderer must skip the frame when the device reports a zero-size window or swapchain resize.

The renderer must not ask the asset manager for raw graphics pointers.

The step is complete when the OpenGL manual model renders through `Rendering::Renderer`.

### Step 16: Migrate the Manual Render Test

Remove direct use of `IRenderer::Submit()` from `ManualRenderTest`.

Create the manual model as a persistent non-entity render instance.

```cpp
instance = renderWorld.CreateMeshInstance(description);
```

Create one render instance for each model mesh. Store the handles in a small owner object.

Apply each model material slot or material override to the related mesh instance.

Update its transform each frame.

```cpp
renderWorld.SetTransform(instance, transform);
```

Destroy the instance before render-world shutdown.

Add a second test that calls `DrawMeshOnce()` each frame.

Compare both images. The persistent and transient paths must give the same material result.

The step is complete when both paths use the same draw-packet code.

### Step 17: Prepare the Editor Integration

The repository does not contain the final editor. Define the editor contract before editor implementation.

Add asset pickers that return asset handles. Do not return pointers or file paths.

Use the optional entity ID for viewport selection and picking.

Use direct render instances for editor grids and persistent editor-only objects.

Use transient render items for one-frame gizmos and selection outlines.

The step is complete when editor operations need no backend API.

### Step 18: Complete the Sprite Path

Create one shared unit quad mesh as a render resource.

Convert each sprite instance into sprite batch data. Do not create one GPU mesh for each sprite.

Use these batch compatibility fields:

- Material
- Texture or texture-array page
- Blend mode
- Render layer
- Render target

Store transform, tint, UV rectangle, and object ID as instance data.

Use a dynamic frame buffer for sprite instance data.

Keep persistent and transient sprites on the same batch path.

The step is complete when persistent and transient sprites can share one draw operation.

### Step 19: Implement Vulkan GPU Resources

Keep the current Vulkan context, device, swapchain, and frame owners where they meet the new interface.

Add these Vulkan backend resources:

- Vulkan buffer with memory ownership
- Vulkan image with memory and image-view ownership
- Vulkan sampler
- Vulkan shader module
- Vulkan descriptor layouts and pools
- Vulkan pipeline layout
- Vulkan graphics pipeline
- Vulkan depth target
- Vulkan upload context

Use staging buffers for device-local mesh and texture data.

Use the resource labels for Vulkan debug names.

Use one Vulkan memory policy. Do not mix manual and allocator ownership for the same resource type.

Implement every graphics-device creation and destruction operation.

The step is complete when Vulkan can create and destroy all fallback resources without validation errors.

### Step 20: Implement Vulkan Draw Packets

Convert the Vulkan clear-frame renderer into `VulkanGraphicsDevice`.

Implement the graphics command list with the active Vulkan command buffer.

Record these operations for each draw packet:

1. Set the graphics pipeline.
2. Set the descriptor sets.
3. Set the vertex buffer.
4. Set the index buffer when present.
5. Set draw constants.
6. Send the draw operation.

Create frame constants for each frame in flight. Do not update a buffer that the GPU still uses.

Create material descriptors with stable resource lifetimes. Do not modify shared material descriptors for object data.

Create or find pipelines with the pipeline key from Step 11.

Add the depth target to dynamic rendering. Recreate it after a size change.

The step is complete when Vulkan renders the same manual model as OpenGL.

### Step 21: Complete Window and Presentation Control

Keep GLFW in the platform layer.

Let the selected backend specify the GLFW client API before window creation.

Move these operations out of general engine code:

- GLAD initialization
- OpenGL debug-output initialization
- OpenGL swap interval
- Vulkan surface creation details
- Vulkan present-mode selection

Map the engine VSync setting to OpenGL swap interval or Vulkan present mode.

Handle these window conditions:

- Resize
- Minimize
- Restore
- Zero framebuffer size
- Full-screen change
- VSync change

The step is complete when both backends survive repeated resize and minimize operations.

### Step 22: Convert ImGui and Editor Rendering

Keep `ImGuiLayer` responsible for the ImGui context and UI construction.

Add one backend integration interface for ImGui frame operations.

Move OpenGL ImGui calls into the OpenGL backend integration.

Add the Vulkan ImGui source files from the same ImGui revision.

Initialize the GLFW backend for the selected graphics backend.

Record Vulkan ImGui data in the active editor render pass.

Keep multi-viewport support off for the first Vulkan implementation. Add it after the main viewport is stable.

Do not let `Engine` include an OpenGL ImGui backend header.

The step is complete when the profiler and console render with both backends.

### Step 23: Define Asset Reload and Resource Destruction

Define this resource lifetime sequence:

```text
Asset import
Asset registration
GPU resource resolution
Render use
Asset reload or unload
GPU resource retirement
GPU-safe destruction
Handle generation change
```

Do not destroy a GPU resource while a submitted frame can use it.

Use a deferred-destruction queue. Key each entry to a completed frame or fence value.

On asset reload, create the new GPU resource first. Replace the cache entry only after successful creation.

Keep the old resource when reload fails. Report the failure once.

On device loss, invalidate all GPU handles. Keep asset handles and source data valid.

The step is complete when repeated load, reload, unload, and shutdown tests have no stale access.

### Step 24: Fix Startup and Shutdown Order

Use this startup order:

```text
Platform and window
Graphics device
Asset manager
Render resource manager
Render world
Renderer
ImGui backend integration
Editor content
```

Use the reverse dependency order for shutdown:

```text
Stop new user submissions
Destroy direct render instances
Finish or cancel transient render items
Finish the active renderer frame
Wait for required GPU work
Shutdown ImGui backend integration
Shutdown the renderer
Shutdown the render world
Destroy render resources
Clear asset data
Shutdown the graphics device
Destroy the window
```

Make each shutdown operation safe for repeated calls.

Add partial-startup cleanup. Test failure after each startup stage.

The step is complete when every startup failure releases its completed stages.

### Step 25: Add Diagnostics and Profiling

Add a label to each render instance, asset, GPU resource, render pass, and pipeline.

Add these frame counters:

- Persistent render instances
- Transient render items
- Visible render items
- Culled render items
- Draw packets
- Draw operations
- Sprite batches
- Pipeline changes
- Material changes
- Buffer uploads
- Texture uploads
- Deferred destructions

Move OpenGL GPU memory estimates into `OpenGLGraphicsDevice`.

Add Vulkan allocation statistics after Vulkan resource ownership exists.

Add stale-handle and wrong-device errors. Include the handle type, index, and generation.

The step is complete when both backends report the same renderer counters.

### Step 26: Add Automated Tests

Add a test target. Do not require a window for unit tests.

Use a test graphics device for renderer tests.

Add these test groups:

- Typed handle tests
- Resource table tests
- Render-instance lifetime tests
- Asset reload tests
- Fallback resource tests
- Queue culling tests
- Queue sorting tests
- Sprite batch tests
- Draw-packet tests
- Startup failure tests
- Shutdown order tests

Add OpenGL integration tests for resource creation and one indexed draw.

Add Vulkan integration tests for resource creation, resize, and one indexed draw.

Run Vulkan integration tests with validation enabled.

Add image checks for the fallback texture, manual model, and sprite batch.

The step is complete when the automated checks cover both user entry paths.

### Step 27: Remove the Old API

Remove these public concepts after all callers use the new API:

- `Graphics::IRenderer`
- Pointer-based `Graphics::DrawCmd`
- Public `Graphics::Mesh::Bind()`
- Public `Graphics::Mesh::Draw()`
- Public `Graphics::Shader::Bind()`
- Public uniform setters
- Public `Graphics::Texture2D::Bind()`
- Public `VertexArray`
- `RenderResourceResolver`
- Asset getters that return graphics pointers
- Backend checks in game and editor code

Keep OpenGL implementation types private under `graphics/opengl/`.

Keep Vulkan implementation types private under `graphics/vulkan/`.

Search for these terms before removal:

```text
glBind
glDraw
glUseProgram
GL_
vk::
Vk
GetId()
Bind()
DrawCmd
IRenderer
```

Only backend files and approved platform integration files can contain backend terms.

The step is complete when the engine builds without the old public headers.

### Step 28: Update the Architecture Documents

Update these documents:

- `docs/Architecture/Graphics Backend.md`
- `docs/Architecture/Render Submission.md`
- `docs/Architecture/System Boundaries.md`
- `docs/Architecture/Asset Manager.md`
- `docs/Architecture/Vulkan Backend.md`
- `docs/Guides/VULKAN_REFACTOR.md`
- `docs/Roadmap.md`

Remove instructions that require raw pointers in `DrawCmd`.

Remove instructions that make asset registries own graphics objects.

Change Vulkan stages so that they implement `VulkanGraphicsDevice`.

Add the final dependency rules:

```text
editor -> rendering
rendering -> assets and graphics interfaces
assets -> import data and file services
graphics interfaces -> no backend
OpenGL backend -> graphics interfaces
Vulkan backend -> graphics interfaces
```

The step is complete when no architecture document describes the old API as the target.

## 6. Required Checkpoints

Do not continue after a failed checkpoint.

1. Existing OpenGL output works through `OpenGLGraphicsDevice`.
2. Asset registries work without a graphics device.
3. Direct persistent instances render with OpenGL.
4. Transient instances render with OpenGL.
5. Persistent and transient sprites use the same batch path.
6. Vulkan creates all fallback resources without validation errors.
7. Vulkan renders the indexed manual model.
8. OpenGL and Vulkan use the same render items and material assets.
9. ImGui works with both backends.
10. Resize, minimize, reload, and shutdown tests pass.
11. The old public graphics API has no caller.

## 7. Final Acceptance Conditions

The conversion is complete only when all conditions are true.

- User code creates the same render instance without an entity.
- User code submits the same description for one frame.
- All paths create the same internal render-item types.
- The renderer owns culling, sorting, batching, and draw-packet creation.
- The render resource manager converts asset handles into GPU handles.
- Asset registries do not own OpenGL or Vulkan objects.
- User headers do not contain OpenGL or Vulkan types.
- OpenGL and Vulkan implement the same graphics-device contract.
- OpenGL and Vulkan render the required reference images.
- Vulkan validation reports no error for the tested path.
- The engine destroys all GPU resources before device shutdown.
- Automated tests cover handles, lifetimes, synchronization, and both backends.

## 8. Work That Must Remain Separate

Do not include these features in the conversion unless a conversion step needs them:

- A full render graph
- Multithreaded command recording
- Asynchronous asset streaming
- GPU-driven culling
- Bindless resources
- Ray tracing
- A general material node editor
- A full editor implementation

Design extension points for these features. Do not delay the core conversion for them.
