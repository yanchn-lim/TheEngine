# Render Submission

## Goal

Convert world data into renderer-agnostic draw commands.

## Current Flow

```text
ManualRenderTest / future Scene / ECS / E-CS
    -> render submission layer
    -> asset handle resolution
    -> Graphics::DrawCmd
    -> Graphics::Material
    -> Graphics::Renderer
    -> OpenGL
```

## Why This Layer Exists

World data and GPU commands should not be the same thing.

The submission layer can handle:

- Visibility checks
- Sorting order
- Texture/material selection
- Transform matrix creation
- Asset handle resolution
- Command creation

## Current Status

The temporary `SpriteRenderSystem` bridge has been removed from the current backend direction.

Manual imported mesh rendering has been moved out of the main engine loop into `ManualRenderTest`. This keeps visual smoke testing available without making `engine.cpp` the permanent render submission layer.

`ManualRenderTest` is not final architecture. It is a temporary app-level harness that loads a model asset, resolves the model's mesh handles and material, and submits `Graphics::DrawCmd` objects to the renderer.

The next architectural step is a render resource access boundary. That boundary should let future Scene, GameObject, ECS, or E-CS code submit renderable data without giving `Graphics::Renderer` direct knowledge of `Assets::AssetManager`.

Current rule:

```text
Renderer receives resolved DrawCmd data.
Renderer does not resolve asset handles.
Renderer does not know about Scene, GameObject, ECS, or AssetManager.
```
