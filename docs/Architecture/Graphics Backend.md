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

## Responsibilities

- Initialize graphics resources
- Own GPU handles safely
- Bind shaders, textures, meshes, and buffers
- Execute submitted draw commands
- Keep frame lifecycle clear with `BeginFrame()` and `EndFrame()`

## Non-Responsibilities

- Owning Scene or GameObject data
- Knowing about SpriteComponent
- Loading arbitrary game assets by path long term
- Deciding gameplay visibility or behavior

## Near-Term Cleanup

- Add renderer diagnostics for invalid draw commands
- Add basic render state handling
- Add a basic Material type after render state exists

## Current Progress

- Renderer no longer owns hardcoded test mesh, shader, or texture resources.
- Sprite-specific rendering has been removed from the backend focus for now.
- Vertex buffer, index buffer, vertex array, and vertex layout wrappers are in place.
- Mesh now has separate creation paths for non-indexed and indexed geometry.
- Mesh owns its GPU buffers and issues either `glDrawArrays()` or `glDrawElements()` depending on whether an index buffer exists.
- Primitive quads now use indexed mesh data.

## Material Direction

Material should group draw-time surface state:

- Shader
- Texture bindings
- Uniform parameters
- Render state

Do not build a large material system yet. Add the first Material type after basic render state handling exists.
