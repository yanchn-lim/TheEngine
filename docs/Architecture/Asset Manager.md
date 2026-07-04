# Asset Manager

## Goal

Load, cache, and retrieve assets through stable handles.

## Initial Asset Types

- Textures
- Shaders
- Materials
- Meshes
- Models

## Handle Direction

Components and scene data should store handles instead of owning GPU resources.

```cpp
struct TextureHandle { AssetId id; };
struct ShaderHandle { AssetId id; };
struct MaterialHandle { AssetId id; };
struct MeshHandle { AssetId id; };
```

Handle id `0` is invalid. Handles expose validity checks so callers can write `if (!handle)`.

## Current Registries

- `Assets::AssetManager`
- `Assets::TextureRegistry`
- `Assets::ShaderRegistry`
- `Assets::MaterialRegistry`

The asset manager owns the current registries and is the public facade used by engine code.

Texture and shader registries:

- Load assets from paths
- Cache path keys to handles
- Store graphics resources by asset id
- Resolve handles to non-owning graphics resource pointers
- Return invalid handles on load failure
- Clear stored resources on shutdown

The material registry:

- Stores `Graphics::Material` objects by asset id
- Maps material names to `MaterialHandle`
- Returns resolved non-owning material pointers
- Does not load material files from disk yet

Planned mesh/model path:

- `Assets::MeshRegistry` owns final `Graphics::Mesh` resources.
- `Assets::ModelLoader` chooses a format importer by file extension or capability.
- Format importers convert third-party library output into engine-owned model data with vertex streams.
- `ObjModelImporter` will use tinyobjloader first.
- Future importers can use glTF, Assimp, or another library without changing graphics code.

## Current Ownership

- Handles are non-owning ids.
- Registries own their stored resources.
- `TextureRegistry` owns `Graphics::Texture2D` resources.
- `ShaderRegistry` owns `Graphics::Shader` resources.
- `MaterialRegistry` owns `Graphics::Material` resources.
- `MeshRegistry` should own `Graphics::Mesh` resources once implemented.
- `Graphics::Material` currently stores resolved non-owning pointers to shader and texture resources.

Because materials point at shader and texture resources, `AssetManager` should clear materials before clearing the registries they reference.

## Responsibilities

- Load assets by path
- Cache loaded assets
- Return stable handles
- Resolve handles to resources
- Provide fallback resources for failed loads

## Current Integration

The engine test now loads shader and texture resources through `Assets::AssetManager`, then creates named materials from those handles and a `Graphics::RenderState`.

Draw commands ask the asset manager for resolved material pointers by name, such as `assets.Get("steak")` and `assets.Get("steak_noblend")`.

Fallback behavior is currently:

- Load failure returns an invalid handle.
- Invalid texture lookups return a generated magenta/black checker texture.
- Invalid shader lookups return a built-in fallback shader matching the current position/color/uv vertex contract.
- Invalid material lookups return a fallback material.
- The renderer does not know about asset fallback behavior.

The renderer still receives resolved `Graphics::DrawCmd` data and does not depend on the asset manager or asset registries directly.

## Model Loading Direction

`ModelLoader` should be extensible and should not be tied directly to tinyobjloader.

Target shape:

```text
file format library
    -> format importer
    -> engine-owned ModelData with vertex streams
    -> MeshRegistry
    -> Graphics::Mesh
```

The first implementation should be:

```text
tinyobjloader
    -> ObjModelImporter
    -> ModelData / ModelMeshData
    -> MeshRegistry
```

The importer interface should hide third-party types. No `tinyobj::` types should appear in `AssetManager`, `MeshRegistry`, `Graphics::Mesh`, `Graphics::MeshData`, scene code, or renderer code.

Use vertex streams instead of a single universal vertex struct.

The model side stores what each stream means:

```text
VertexSemantic::Position
VertexSemantic::Normal
VertexSemantic::Tangent
VertexSemantic::Color
VertexSemantic::TexCoord0
VertexSemantic::TexCoord1
VertexSemantic::BoneIds
VertexSemantic::BoneWeights
```

Each stream also stores its data type and byte data. `MeshRegistry` should pack those streams into interleaved vertex bytes and generate a `Graphics::VertexLayout` before creating a `Graphics::Mesh`.

Target model mesh shape:

```text
ModelMeshData
    vertexCount
    streams
    indices
    topology
```

Keep semantic and shader location separate:

```text
Semantic = what the data means
Location = where the current shader expects that data
```

Current default location convention:

```text
Position -> location 0
Color -> location 1
TexCoord0 -> location 2
```

OBJ files do not reliably provide vertex colors, so the OBJ importer can emit a default white color stream when needed. If UVs are missing, it can emit a default `{0, 0}` TexCoord0 stream until material/texture import is more complete.

## Open Decision

The current direction is Option B:

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

The open work is to turn this into a cleaner render resource access boundary instead of resolving materials directly inside temporary manual test code.

## Future Material Loading

Materials are currently created in code. A future material asset path should describe:

- Shader reference
- Texture references
- Render state
- Uniform values

The material loader can then resolve referenced shader and texture handles through the asset manager before creating the runtime `Graphics::Material`.
