# Asset Manager

## Goal

Load, cache, and retrieve assets through stable handles.

## Initial Asset Types

- Textures
- Shaders
- Materials
- Meshes or primitive meshes

## Handle Direction

Components and scene data should store handles instead of owning GPU resources.

```cpp
using TextureHandle = uint32_t;
using ShaderHandle = uint32_t;
using MaterialHandle = uint32_t;
using MeshHandle = uint32_t;
```

Reserve handle `0` as invalid.

## Responsibilities

- Load assets by path
- Cache loaded assets
- Return stable handles
- Resolve handles to resources
- Provide fallback resources for failed loads

## Open Decision

Decide where handle resolution happens:

Option A:

```text
DrawCmd stores handles
Renderer resolves handles
```

Option B:

```text
Render submission resolves handles
DrawCmd stores resolved pointers
Renderer only draws
```

Option B keeps Renderer more independent from the asset manager.
