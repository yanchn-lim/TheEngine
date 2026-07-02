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
- [ ] Index buffer support
- [ ] Vertex layout abstraction
- [ ] Renderer resource naming cleanup
- [ ] Remove or isolate test-only renderer resources
- [ ] Add renderer diagnostics for invalid draw commands
- [ ] Add basic render state handling

## Phase 2 - Asset Manager

Goal: load, cache, and retrieve assets without coupling to Scene, GameObject, or ECS.

- [ ] Define asset handle types
- [ ] Define invalid handle constants
- [ ] Add TextureRegistry or AssetManager texture path
- [ ] Add ShaderRegistry or AssetManager shader path
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
- [x] SpriteRenderSystem bridge
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
