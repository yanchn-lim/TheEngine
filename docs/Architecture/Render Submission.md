# Render Submission

`RenderWorld` is the single front-facing render submission API.

Persistent entity components, persistent non-entity objects, and one-frame code draws use `MeshInstanceDesc` or `SpriteInstanceDesc`. Persistent calls return a generation-checked `RenderInstanceHandle`. Transient calls copy the description into frame storage.

`Systems::RenderSystem` processes ECS render components. It converts component and transform lifecycle events into add, update, or remove operations on `RenderWorld`. It does not create GPU resources and it does not send device commands.

At frame start, `Rendering::Renderer` collects visible items, applies layer filtering and stable sort order, resolves their asset handles, and sends backend-neutral commands. Both back ends receive the same resolved draw data.

When an editor or asset tool replaces mesh, texture, shader, or material data, it must call the matching `Renderer::Invalidate` overload outside an active frame. The renderer waits for in-flight work and removes dependent cached GPU resources. The next visible draw uploads the current asset data again.
