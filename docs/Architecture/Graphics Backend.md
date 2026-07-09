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
- `Graphics::VertexArray`
- `Graphics::VertexBuffer`
- `Graphics::IndexBuffer`
- `Graphics::VertexLayout`
- `Graphics::MeshData`
- `Graphics::PrimitiveTopology`
- `Graphics::BlendMode`
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
- Generating user-facing primitive mesh assets
- Knowing about model file formats or importer libraries
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
- Mesh data now carries primitive topology, and mesh drawing maps it to OpenGL draw modes.
- Line topology was verified with a quad outline manual test.
- Render state now uses `BlendMode` instead of a raw blending boolean.
- Current blend modes are none, alpha, additive, premultiplied alpha, and multiply.
- Renderer remains independent from `Assets::AssetManager`; asset handles are resolved before draw commands reach the renderer.
- Model importers should convert source data into engine-standard vertex formats before graphics resources are created.
- Mesh creation still consumes interleaved vertex bytes plus the matching `Graphics::VertexLayout`.
- Primitive mesh generation now lives under `Assets::Primitive2D` and returns `Assets::MeshSourceData`.
- Imported OBJ meshes are now converted by the asset layer into `Assets::MeshSourceData`, then uploaded through `MeshRegistry`.
- Imported OBJ geometry has been validated with public OBJ model files.
- The graphics backend remains unaware of tinyobjloader, OBJ files, and model importer details.

## Primitive Topology Direction

Use `PrimitiveTopology` instead of `DrawMode`.

Primitive topology describes how vertices are assembled into primitives:

- Triangles
- Lines
- Points

The name avoids confusion with future mode concepts like blend mode, cull mode, depth mode, and polygon mode.

## Blend Mode Direction

Blend mode controls how source pixels combine with pixels already in the framebuffer.

Current modes:

- None
- Alpha
- Additive
- Premultiplied alpha
- Multiply

Keep the exposed set small. Add more modes only when materials or tools need them.

## Material Direction

Material currently groups draw-time surface state:

- Shader
- Texture bindings
- Render state

Materials currently store resolved non-owning resource pointers. Asset handles belong on the asset/scene side until a dedicated render resource boundary exists.

Future material work should add uniform parameters and file-backed material loading. Do not build a large material system until the asset manager direction is clearer.
