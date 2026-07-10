# To Do - Render Resource Resolver

## Purpose

Create a small render-facing boundary that resolves asset handles into the graphics resources needed to build `Graphics::DrawCmd` objects.

The resolver exists so submission code can work with `Assets::ModelHandle`, `Assets::MeshHandle`, and `Assets::MaterialHandle` without teaching `Graphics::Renderer` about `Assets::AssetManager`.

Target ownership and dependency direction:

```text
ManualRenderTest / future Scene / ECS / E-CS
    -> Rendering::RenderResourceResolver
    -> Assets::AssetManager
    -> Graphics::Mesh / Graphics::Material
    -> Graphics::DrawCmd
    -> Graphics::Renderer
```

The renderer stays at the end of this chain. It receives already-resolved pointers in a draw command and must not include or call asset code.

## Completion Criteria

- [ ] `Rendering::RenderResourceResolver` exists under `GameEngine/src/rendering/`.
- [ ] It holds a non-owning reference to `const Assets::AssetManager`.
- [ ] It resolves model, mesh, and material handles.
- [ ] `ManualRenderTest` uses the resolver instead of calling `assets.Get(...)` directly.
- [ ] The renderer remains independent of `Assets::AssetManager`, Scene, GameObject, ECS, and E-CS.
- [ ] The existing manual model test still draws correctly.
- [ ] Debug x64 builds with no warnings or errors.

## Design Rules

Keep the first version deliberately small.

- The resolver does not own assets, meshes, materials, shaders, or textures.
- The resolver does not cache pointers yet.
- The resolver does not submit draw commands.
- The resolver does not sort, batch, cull, or manage frame queues.
- The resolver does not create a dependency from the graphics backend back to assets.
- Invalid handles use the asset manager's existing fallback/null behavior. Do not duplicate fallback policy inside the resolver.

The resolver is a boundary, not another asset registry and not a new renderer.

## Step 1 - Create the Rendering Folder

Create this folder:

```text
GameEngine/src/rendering/
```

This is a suitable home for code that converts higher-level render intent into `Graphics::DrawCmd` data. It is separate from:

- `graphics/`, which owns GPU-facing resource types and the renderer.
- `assets/`, which owns loading, registries, handles, and fallback resources.
- `scene/`, which will eventually own world/object data.

Add the new file to the Visual Studio project and put it in a matching `rendering` filter. Keep it header-only for now because it is a small forwarding wrapper.

## Step 2 - Define the Resolver

Create:

```text
GameEngine/src/rendering/render_resource_resolver.hpp
```

Its public shape should be close to this:

```cpp
namespace Rendering
{
    class RenderResourceResolver
    {
    public:
        explicit RenderResourceResolver(const Assets::AssetManager& assets);

        const Assets::ModelAsset* Resolve(Assets::ModelHandle handle) const;
        const Graphics::Mesh* Resolve(Assets::MeshHandle handle) const;
        const Graphics::Material* Resolve(Assets::MaterialHandle handle) const;

    private:
        const Assets::AssetManager& _assets;
    };
}
```

Use `Resolve(...)` rather than `Get(...)` because the class is translating a handle into a render-usable resource. `AssetManager::Get(...)` can remain the low-level registry access API.

Use a reference, not a pointer and not a smart pointer:

```cpp
const Assets::AssetManager& _assets;
```

The engine owns the asset manager for longer than the temporary resolver instance. The resolver only borrows access during render submission, so it should not participate in ownership.

## Step 3 - Implement the Forwarding Functions

Each resolver function should only forward to the corresponding asset-manager lookup:

```cpp
const Graphics::Mesh* Resolve(Assets::MeshHandle handle) const
{
    return _assets.Get(handle);
}
```

Apply the same pattern for models and materials. Avoid adding validation, logging, maps, or special fallback logic here. Existing registries and `AssetManager` already define that behavior.

Use explicit includes for the public types the header exposes. A straightforward first version can include:

```cpp
#include "assets/asset_manager.hpp"
#include "assets/model_asset.hpp"
#include "graphics/material.hpp"
#include "graphics/mesh.hpp"
```

Later, this may be reduced with forward declarations, but clarity is more valuable than minimizing includes at this stage.

## Step 4 - Use It in ManualRenderTest

In `ManualRenderTest::Submit`, construct a short-lived resolver from the existing `assets` reference:

```cpp
Rendering::RenderResourceResolver resources(assets);
```

Then replace direct asset-manager access in that function:

```cpp
const Assets::ModelAsset* modelAsset = resources.Resolve(model);
const Graphics::Material* modelMaterial = resources.Resolve(material);

for (const Assets::MeshHandle mesh : modelAsset->meshes)
{
    Graphics::DrawCmd cmd;
    cmd.mesh = resources.Resolve(mesh);
    cmd.material = modelMaterial;
    // Set transform and submit as before.
}
```

The transform calculation and `renderer.Submit(cmd)` call remain where they are. This task only changes where resources are resolved.

## Step 5 - Check Dependency Direction

After the change, confirm these constraints:

```text
rendering -> assets
rendering -> graphics
assets    -> graphics
graphics  -X-> assets
graphics  -X-> rendering
```

In practical terms:

- `Graphics::Renderer` should not take an `AssetManager` parameter.
- `Graphics::DrawCmd` should still contain resolved `Graphics::Mesh*` and `Graphics::Material*` pointers.
- `AssetManager` should not include `rendering/render_resource_resolver.hpp`.
- Scene/ECS code will eventually depend on `rendering`, but `rendering` should not depend on Scene/ECS.

## Step 6 - Verify

Build the Debug x64 solution:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' GameEngine\GameEngine.sln /p:Configuration=Debug /p:Platform=x64 /m
```

Run the engine and verify that the current manual OBJ model test still renders with its assigned material and texture.

Also verify that an invalid or missing resource follows the existing fallback/null path. The resolver should not change the visible behavior; it only makes the access boundary explicit.

## Do Not Add Yet

Keep these as later rendering-submission tasks:

- Render queues
- Command sorting
- Material batching
- Frustum culling
- Frame-ring buffering
- Scene traversal
- Sprite-specific systems
- ECS systems
- Resolver-side resource caching

Those features will have a better home once the basic resource-access boundary is proven and the submission API is clearer.

## After This Task

Once the resolver is built and the dependency direction has been checked, add a small handle-based render submission type. This should become the stable input that future Scene, GameObject, ECS, or E-CS code produces.

## Follow-Up - Handle-Based Render Submission

### Purpose

Define render intent without exposing raw `Graphics::Mesh*` or `Graphics::Material*` pointers to world-level code.

The type should contain asset handles and the per-instance data required for one draw:

```cpp
namespace Rendering
{
    struct RenderItem
    {
        Assets::MeshHandle mesh;
        Assets::MaterialHandle material;
        glm::mat4 transform;
    };
}
```

The final flow should be:

```text
ManualRenderTest / future Scene / ECS / E-CS
    -> Rendering::RenderItem (handles + transform)
    -> Rendering::RenderResourceResolver
    -> Graphics::DrawCmd (resolved resource pointers)
    -> Graphics::Renderer
```

### To Do

- [ ] Create `render_item.hpp` under `GameEngine/src/rendering/`.
- [ ] Store only `MeshHandle`, `MaterialHandle`, and a transform in the initial type.
- [ ] Keep it a plain data type with no renderer, asset-manager, scene, or ECS dependency.
- [ ] Add a resolver function that converts a valid `RenderItem` into a `Graphics::DrawCmd`.
- [ ] Define the invalid-resource result clearly: return `false` with an output draw command, or return an optional draw command. Choose one convention and use it consistently.
- [ ] Change `ManualRenderTest` to create `RenderItem` values, then resolve them immediately before `renderer.Submit(...)`.
- [ ] Verify the existing model still renders without `ManualRenderTest` constructing a `Graphics::DrawCmd` directly.

### Boundaries

`RenderItem` is not a scene component and should not contain object IDs, visibility state, sorting keys, animation data, camera data, or asset ownership.

`Graphics::DrawCmd` remains graphics-backend data. It keeps resolved graphics resource pointers because the renderer should not perform asset lookups.

The resolver is the only code that needs to understand both `RenderItem` and `Graphics::DrawCmd`.

### Defer Until Later

- Render-item queues
- Sorting keys and batching
- Culling
- Model-to-many-mesh expansion
- Sprite-specific submission
- Scene traversal
- ECS scheduling

Implement the single-item path first. A queue can be introduced later without changing the graphics backend when there is a real source of many items to submit.

## Then Continue With Assets

After the single-item submission path works, return to the remaining asset-manager work:

- [ ] Add OBJ/MTL material import support, if OBJ materials are part of the intended asset pipeline.
- [ ] Decide and document the unload/reload policy for registries and handles.
