# Graphics API

## User API

Use `Rendering::RenderWorld` for all world drawing. Do not select a graphics backend in game, editor, scene, or ECS code.

Create a persistent mesh without an entity:

```cpp
Rendering::MeshInstanceDesc mesh;
mesh.mesh = meshAsset;
mesh.material = materialAsset;
mesh.transform = transform;
Rendering::RenderInstanceHandle instance = renderWorld.CreateMeshInstance(mesh);
```

Use the same description for a one-frame draw:

```cpp
renderWorld.DrawMeshOnce(mesh);
```

Use `CreateSpriteInstance()` and `DrawSpriteOnce()` in the same way for sprites. `RenderWorld` copies transient descriptions and clears them after the frame.

An ECS or editor component does not send a low-level draw command. `Systems::RenderSystem` adds, updates, or removes the same persistent render instance that direct code creates. A component stores asset handles and a runtime-only render-instance handle. It does not store a GPU handle.

## Ownership and Calls

- `AssetManager` owns CPU asset records.
- `RenderWorld` owns persistent and transient render items.
- `RenderResourceManager` owns the asset-to-GPU cache.
- `IGraphicsDevice` owns GPU resources.
- The selected back end owns native OpenGL or Vulkan objects.

Call `RenderWorld` from game or editor integration code. Call `IGraphicsDevice` only from rendering code on the render thread. A persistent handle is valid until `Destroy()` or `Clear()`. A transient item is valid until the renderer completes the current frame.

Use `--opengl` or `--vulkan` to select a back end. Vulkan is the default.

The editor UI uses a backend integration interface. OpenGL supports ImGui platform viewports. Vulkan renders docking UI in the main viewport and keeps platform viewports disabled until the engine owns a Vulkan swapchain for each platform window.

Run `GameEngine/tools/compile_shaders.ps1` after a Vulkan shader source change. The script uses `glslc.exe` from `VULKAN_SDK` and replaces the checked-in SPIR-V files.
