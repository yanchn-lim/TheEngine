# Graphics Backend

## Goal

Provide reusable GPU-facing rendering primitives and execute draw commands.

## Current Types

- `Graphics::Renderer`
- `Graphics::DrawCmd`
- `Graphics::Shader`
- `Graphics::Texture2D`
- `Graphics::Mesh`
- `Graphics::Camera2D`
- `Graphics::Primitive2D`
- `Graphics::VertexArray`
- `Graphics::VertexBuffer`
- `Graphics::IndexBuffer`
- `Graphics::VertexLayout`
- `Graphics::MeshData`
- `Graphics::RenderState`
- `Graphics::Material`

## Responsibilities

- Initialize graphics resources
- Own GPU handles safely
- Bind materials, shaders, textures, meshes, and buffers
- Execute submitted draw commands
- Keep frame lifecycle clear with `BeginFrame()` and `EndFrame()`

## Non-Responsibilities

- Owning Scene or GameObject data
- Knowing about SpriteComponent
- Loading arbitrary game assets by path long term
- Deciding gameplay visibility or behavior

## Near-Term Cleanup

- Decide the next backend feature before asset manager

## Current Progress

- Renderer no longer owns hardcoded test mesh, shader, or texture resources.
- Sprite-specific rendering has been removed from the backend focus for now.
- Vertex buffer, index buffer, vertex array, and vertex layout wrappers are in place.
- Mesh now has separate creation paths for non-indexed and indexed geometry.
- Mesh owns its GPU buffers and issues either `glDrawArrays()` or `glDrawElements()` depending on whether an index buffer exists.
- Primitive quads now use indexed mesh data.
- Draw commands now reference a material instead of separate shader, texture, and render state fields.
- Renderer validates material, shader, and mesh before drawing.
- Renderer applies basic render state: blending, depth test, depth write, and culling.
- Manual material test submits two draw commands with different material render states.
- Blending test verified expected behavior: alpha respects transparency when blending is enabled, and transparent pixels render as their raw color when blending is disabled.

## Material Direction

Material currently groups draw-time surface state:

- Shader
- Texture bindings
- Render state

Future material work should add uniform parameters and asset-manager ownership/lookup. Do not build a large material system until the asset manager direction is clear.
