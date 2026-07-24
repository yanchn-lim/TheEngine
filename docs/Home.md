# TheEngine Docs

This vault tracks engine architecture, implementation order, and system boundaries.

## Start Here

- [[Roadmap]]
- [[Architecture/System Boundaries]]
- [[Architecture/Graphics Backend]]
- [[Architecture/Vulkan Backend]]
- [[Guides/Vulkan Implementation Tutorial]]
- [[Architecture/Memory Profiler]]
- [[Architecture/Asset Manager]]
- [[Architecture/Render Submission]]
- [[To Do/Render Resource Resolver|Render Resource Resolver To-Do]]
- [[Canvases/Engine Systems.canvas|Engine Systems Canvas]]

## Current Direction

The engine keeps graphics, assets, and ECS storage independent. Integration adapters connect their public APIs without adding backend dependencies to gameplay or ECS code.

The immediate implementation goal is:

1. Finish a reliable graphics backend.
2. Build a reusable asset manager.
3. Integrate asset resolution with render submission.
4. Build gameplay systems on `ECS::World` views and typed queries.

## Current Focus

Vulkan backend implementation is the active focus until complete. The bounded CPU/OpenGL estimated-memory profiler is complete, so work now returns to the backend-neutral renderer/resource seam. See [[Architecture/Memory Profiler]], [[Architecture/Vulkan Backend]], and [[Roadmap]].
