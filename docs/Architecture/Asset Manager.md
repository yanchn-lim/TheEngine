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
- `Assets::MeshRegistry`

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

The mesh registry:

- Stores final `Graphics::Mesh` resources by asset id
- Creates meshes from `Assets::MeshSourceData`
- Uses the standard `Assets::MeshVertex` layout
- Maps mesh names to `MeshHandle`
- Returns resolved non-owning mesh pointers

Current mesh/model path:

- `Assets::MeshRegistry` owns final `Graphics::Mesh` resources.
- `Assets::ModelLoader` chooses a format importer by file extension or capability.
- Format importers convert third-party library output into engine-standard mesh layouts.
- `ObjModelImporter` uses tinyobjloader for the first supported model format.
- Future importers can use glTF, Assimp, or another library without changing graphics code.

## Current Ownership

- Handles are non-owning ids.
- Registries own their stored resources.
- `TextureRegistry` owns `Graphics::Texture2D` resources.
- `ShaderRegistry` owns `Graphics::Shader` resources.
- `MaterialRegistry` owns `Graphics::Material` resources.
- `MeshRegistry` owns `Graphics::Mesh` resources.
- `Graphics::Material` currently stores resolved non-owning pointers to shader and texture resources.

Because materials point at shader and texture resources, `AssetManager` should clear materials before clearing the registries they reference.

## Responsibilities

- Load assets by path
- Cache loaded assets
- Return stable handles
- Resolve handles to resources
- Provide fallback resources for failed loads

## Current Integration

The engine test now loads shader and texture resources through `Assets::AssetManager`, creates named materials from those handles and a `Graphics::RenderState`, and creates a reusable quad mesh through `Assets::Primitive2D::Quad()`.

Draw commands ask the asset manager for resolved material pointers by name, such as `assets.Get("steak")` and `assets.Get("steak_noblend")`, and resolve mesh handles through `assets.Get(meshHandle)`.

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
    -> engine-owned model data in a standard vertex format
    -> MeshRegistry
    -> Graphics::Mesh
```

The current OBJ implementation is:

```text
tinyobjloader
    -> ObjModelImporter
    -> MeshSourceCollection / MeshSourceData
    -> MeshRegistry
```

The importer currently supports:

- Positions
- UVs
- White default vertex color
- Missing UVs as `{0, 0}`
- One `MeshSourceData` per OBJ shape
- Vertex deduplication by OBJ position/uv/normal index triplet
- Multi-mesh `AssetManager::LoadModel(name, path)` loading
- Public OBJ model geometry import validation

The importer does not yet support:

- Normals in the runtime vertex format
- Tangents
- OBJ material/MTL import

The importer interface should hide third-party types. No `tinyobj::` types should appear in `AssetManager`, `MeshRegistry`, `Graphics::Mesh`, `Graphics::MeshData`, scene code, or renderer code.

Use standard engine vertex format contracts instead of arbitrary vertex streams.

The first supported format is `MeshVertex`:

```text
location 0: position  float3
location 1: color     float3
location 2: texcoord0 float2
```

The matching shader must declare the same layout:

```glsl
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;
```

OBJ import should convert source data into this format. Missing color becomes white. Missing UV becomes `{0, 0}`. If a future shader needs normals or tangents, add a new standard format such as `LitMeshVertex` instead of making every shader accept every possible attribute.

OBJ files have separate indices for position, UV, and normal. The importer converts those triplets into the engine's single packed `MeshVertex` format. Exact repeated triplets are deduplicated into shared indexed vertices. Vertices with the same position but different UVs or normals remain separate, preserving UV seams and hard edges.

`AssetManager::LoadMesh(name, path)` remains a convenience path that creates one mesh from the first entry in the resulting `MeshSourceCollection`. `AssetManager::LoadModel(name, path)` preserves multi-shape files by creating one mesh handle per `MeshSourceData` entry using generated child names.

Successful OBJ import logs are condensed into a single verbose summary per file with mesh, vertex, and index counts. Import warnings and errors remain normal warning/error logs.

Future standard formats can be added as needed:

```text
MeshVertex:
    position, color, texcoord0

LitMeshVertex:
    position, normal, tangent, texcoord0

SkinnedMeshVertex:
    position, normal, tangent, texcoord0, bone indices, bone weights
```

Each standard format should own:

- A C++ vertex struct
- A function that builds the matching `Graphics::VertexLayout`
- Matching shader input layout conventions

## Built-In Primitive Meshes

Reusable primitive shapes live on the asset side:

```text
Assets::Primitive2D::Triangle()
Assets::Primitive2D::Quad()
Assets::Primitive2D::Circle()
```

These functions return `Assets::MeshSourceData`. They do not create GPU resources directly.

Expected flow:

```text
Assets::Primitive2D::Quad()
    -> MeshSourceData
    -> AssetManager::CreateMesh()
    -> MeshRegistry
    -> MeshHandle
```

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
