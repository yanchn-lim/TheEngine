# Roadmap

## Phase 1 - Graphics Backend

Goal: own GPU-facing rendering code behind reusable graphics types.

- [x] Basic window and OpenGL initialization
- [x] Shader class
- [x] Texture2D class
- [x] Mesh class
- [x] Draw command container
- [x] Camera2D
- [x] Remove graphics-side Primitive2D mesh data path
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
- [x] Decide next backend feature before asset manager

## Phase 2 - Asset Manager

Goal: load, cache, and retrieve assets without coupling to Scene, GameObject, or ECS.

- [x] Define asset handle types
- [x] Define invalid handle constants
- [x] Add AssetManager facade over asset registries
- [x] Add TextureRegistry texture path
- [x] Add ShaderRegistry shader path
- [x] Add MaterialRegistry material path
- [x] Add named material lookup
- [x] Add MeshRegistry for mesh resources
- [x] Add standard engine vertex format definitions
- [x] Add `MeshVertex` and matching `VertexLayout` builder
- [x] Add asset-side `Primitive2D` mesh generators
- [ ] Convert imported model data into standard mesh layouts
- [ ] Add extensible model importer interface
- [ ] Add OBJ importer using tinyobjloader
- [ ] Keep third-party model loader types out of graphics and registries
- [x] Add path-to-handle caching
- [x] Add handle-to-resource lookup
- [x] Define current registry ownership model
- [ ] Decide full unload/reload policy
- [x] Decide current error/fallback asset policy

## Phase 3 - Graphics and Asset Integration

Goal: connect asset handles to renderable GPU resources cleanly.

- [x] Decide current DrawCmd resource shape: resolved pointers
- [ ] Create render resource access boundary
- [x] Replace renderer-owned fallback test resources
- [x] Add fallback shader, texture, and material path
- [x] Verify fallback resources with manual engine test
- [ ] Let sprite submission resolve texture handles
- [x] Let shader resources come from asset/resource layer
- [x] Let primitive quad mesh resources come from asset/resource layer
- [ ] Let imported mesh resources come from asset/resource layer
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
