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

- Rename or isolate `_testMesh`, `_testShader`, and `_testTexture`
- Replace `GetTestMeshCmd()` with real resource access
- Add index buffer support
- Add a vertex layout abstraction
