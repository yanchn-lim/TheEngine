# Roadmap

## Complete

- [x] Add a single `RenderWorld` API for persistent and one-frame mesh and sprite submission.
- [x] Add generation-checked render-instance and GPU handles.
- [x] Separate CPU asset records from GPU resources.
- [x] Add `RenderResourceManager` for asset-to-GPU resolution.
- [x] Add a shared renderer and command-list contract.
- [x] Convert OpenGL resource ownership, drawing, and presentation to `OpenGLGraphicsDevice`.
- [x] Convert Vulkan resource ownership, drawing, synchronization, and presentation to `VulkanGraphicsDevice`.
- [x] Add an ECS render synchronization adapter.
- [x] Remove the pointer-based renderer and graphics resource API.
- [x] Add OpenGL and Vulkan command-line selection.
- [x] Add Vulkan ImGui rendering with dynamic rendering and shared frame submission.

## Next

- [ ] Add sprite instancing and compatible-item batching.
- [ ] Add depth targets and depth pipeline state to the shared contract.
- [ ] Add deferred GPU destruction for live asset reload.
- [ ] Add frustum culling, render views, and expanded frame counters.
- [ ] Move self-tests into a separate headless test target.
