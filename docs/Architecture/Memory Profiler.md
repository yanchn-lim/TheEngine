# Memory Profiler

## Status

CPU and OpenGL resource profiling are complete. The profiler is intentionally small: it provides reliable C++ heap churn and named OpenGL storage estimates without becoming a custom allocator or leak debugger.

## Scope

Track C++ allocations routed through global `new` and `delete`:

- Current allocated bytes.
- Peak allocated bytes.
- Live allocation count.
- Per-frame allocated/freed bytes and allocation/free counts.

Also track named resource estimates in the `GPU Estimated` domain. Matching `(label, domain)` pairs aggregate their current bytes, peak bytes, and resource count. Rows disappear once all matching resources are released.

## Initial Design

```text
Global C++17 new/delete overloads
    -> allocation header storing raw pointer and requested size
    -> atomic CPU counters
    -> per-frame snapshot before ImGui

OpenGL wrappers
    -> move-only ResourceUsage member
    -> (GPU Estimated, label, byte estimate) aggregate
    -> ProfilerUI memory section
```

The allocation header makes frees exact without a pointer map. Counter updates use atomics and do not allocate. The tracker state is intentionally never destroyed so global deallocation remains safe during CRT teardown.

`ResourceUsage` is move-only, so moved profiler records retain one registration and destruction removes it exactly once. GPU resource ownership now stays inside the selected graphics device.

## Required Correctness Rules

- Every tracked successful allocation contributes exactly once.
- Every matching deallocation removes the same size exactly once.
- Failed allocations do not alter live-byte statistics.
- Resource rows use caller labels and aggregate matching live resources.
- Startup and shutdown allocations remain safe while allocation overrides are active.
- Aligned allocation and deallocation overloads are supported, not only basic `new` and `delete`.
- Array allocation/deallocation overloads are supported.
- `ENGINE_ENABLE_MEMORY_PROFILER` is enabled in both engine Debug and Release builds. A future game-export target owns stripping engine tools and instrumentation.

Do not store a dynamically allocated record for every allocation in the first version. Aggregate counters are enough and avoid the profiler materially changing the allocation pattern it measures.

## Implementation Order

1. `src/debug/memory_tracker.*` exposes CPU snapshots, resource snapshots, and `ResourceUsage`.
2. `memory_overrides.cpp` implements scalar, array, sized, aligned, and nothrow C++17 new/delete overloads when enabled.
3. The engine begins CPU-frame tracking before gameplay/render work and snapshots it after the renderer finishes, before ImGui begins.
4. `ProfilerUI` presents a frame-analysis workspace: interactive frame history, an expandable selected-frame scope tree, recent scope-cost aggregates, and a memory pane with C++ heap totals, last-frame churn, a live-memory trend, and a descending GPU Estimated resource table.
5. Vertex buffers, index buffers, and one-level RGB8/RGBA8 textures are covered. Mesh registry names label mesh buffers; texture paths label textures.

## Explicitly Deferred

- Per-allocation call stacks or symbolization.
- Leak reports with individual allocation records.
- Replacing STL allocators.
- Pool, arena, frame, or custom allocator systems.
- Driver memory telemetry or OpenGL allocation queries.
- Vulkan memory accounting, which will use the same resource table with exact engine-recorded allocation sizes.

## OpenGL Estimates

The same profiler UI can later show backend-specific GPU memory values, but the data quality differs:

```text
OpenGL: engine-estimated buffer/texture/render-target storage
Vulkan: engine-recorded allocation sizes and memory heap/type data
```

OpenGL values are one-level storage estimates, labeled `GPU Estimated`: vertex-buffer upload size, `indexCount * sizeof(uint32_t)`, and texture dimensions multiplied by the actual RGB8 or RGBA8 channel count. Vulkan allocation sizes will later come from `VulkanBuffer` and `VulkanImage` ownership wrappers, with optional device-budget telemetry later.

## Completion Criteria

- [x] CPU allocations and frees are counted with allocation headers and C++17 overload coverage.
- [x] Live/peak bytes, live allocation count, and last-frame allocation/free churn are visible in `ProfilerUI`.
- [x] Named GPU Estimated rows cover vertex buffers, index buffers, and RGB8/RGBA8 textures.
- [x] Resource usage transfers through buffer and texture moves and is released on destruction.
- [x] Debug x64, Release x64, and Release opt-in builds pass.
- [x] The tracker has no dependency on graphics, assets, Scene, ECS, ImGui, or `Profiler`.
- [x] The profiler UI keeps scope hierarchy visible through an expandable selected-frame tree while retaining per-scope timing and share-of-frame data.

This completes the memory-profiler milestone. Do not expand it before a Vulkan buffer/image ownership path exists; Vulkan can later add exact engine-recorded allocation sizes through the existing resource-accounting and UI boundary.
