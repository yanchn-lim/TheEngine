# Render Submission

## Goal

Convert world data into renderer-agnostic draw commands.

## Current Flow

```text
Scene
SpriteRenderSystem
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

## Current Temporary Bridge

`SpriteRenderSystem` uses `Graphics::SpriteRenderResources` from the renderer. This keeps the current scene rendering test working before the asset manager exists.

This should later be replaced by asset/resource manager integration.
