# Roadmap

## Phase 1 - Graphics Backend

Goal: own GPU-facing rendering code behind reusable graphics types.

- [x] Basic window and OpenGL initialization
- [x] Shader class
- [x] Texture2D class
- [x] Mesh class
- [x] Draw command container
- [x] Camera2D
- [x] Primitive2D mesh data
- [x] Vertex buffer wrapper
- [x] Index buffer wrapper
- [x] Vertex array wrapper
- [x] Vertex layout abstraction
- [x] Finish mesh support for indexed and non-indexed drawing
- [x] Add `MeshData` and `Mesh::Create(const MeshData&)`
- [x] Remove sprite-specific renderer/system path from backend focus
- [x] Remove renderer-owned test resources
- [x] Add renderer diagnostics for invalid draw commands
- [x] Add basic render state handling
- [x] Add basic Material type for shader, texture, and render state grouping
- [x] Verify indexed mesh drawing with a manual test
- [x] Add simple material/manual render state test cases
- [x] Add primitive topology support for triangles, lines, and points
- [x] Verify line topology with a manual quad outline test
- [x] Add BlendMode support to render state
- [ ] Decide next backend feature before asset manager

## Phase 2 - Asset Manager

Goal: load, cache, and retrieve assets without coupling to Scene, GameObject, or ECS.

- [ ] Define asset handle types
- [ ] Define invalid handle constants
- [ ] Add TextureRegistry or AssetManager texture path
- [ ] Add ShaderRegistry or AssetManager shader path
- [ ] Add MaterialRegistry or AssetManager material path
- [ ] Add MeshRegistry or primitive mesh cache
- [ ] Add path-to-handle caching
- [ ] Add handle-to-resource lookup
- [ ] Decide ownership and unload policy
- [ ] Decide error/fallback asset policy

## Phase 3 - Graphics and Asset Integration

Goal: connect asset handles to renderable GPU resources cleanly.

- [ ] Decide whether DrawCmd stores handles or resolved pointers
- [ ] Create render resource access boundary
- [ ] Replace renderer-owned fallback test resources
- [ ] Let sprite submission resolve texture handles
- [ ] Let shader and mesh resources come from asset/resource layer
- [ ] Keep Renderer independent from Scene/GameObject/ECS

## Phase 4 - Scene and Object Layer

Goal: keep world structure separate from graphics and assets.

- [x] GameObject stub
- [x] Scene object storage
- [x] Transform2D
- [x] SpriteComponent
- [x] Removed temporary SpriteRenderSystem bridge from current backend direction
- [ ] Scene lifetime rules
- [ ] Object creation/destruction API
- [ ] Stable object handles or IDs
- [ ] Component ownership decision
- [ ] Scene serialization direction

## Phase 5 - Future ECS or E-CS

Goal: make architecture migration possible without rewriting graphics or assets.

- [ ] Define what must remain backend-agnostic
- [ ] Define component data rules
- [ ] Define system scheduling rules
- [ ] Decide GameObject facade vs pure entity IDs
- [ ] Prototype entity/component storage separately
