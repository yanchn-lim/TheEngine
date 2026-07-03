# Render Submission

## Goal

Convert world data into renderer-agnostic draw commands.

## Current Flow

```text
Future Scene / ECS / E-CS
Render submission layer
Graphics::DrawCmd
Graphics::Renderer
OpenGL
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

Render submission should stay as a future layer above graphics and assets. For now, the focus is finishing the reusable graphics backend first, then the asset manager, then connecting both through clean draw command/resource resolution.
