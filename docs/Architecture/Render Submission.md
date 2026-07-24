# Render Submission

`RenderWorld` is the single front-facing render submission API.

Persistent objects and one-frame code draws use `MeshInstanceDesc` or `SpriteInstanceDesc`. Persistent calls return a generation-checked `RenderInstanceHandle`. Transient calls copy the description into frame storage.

The current ECS-to-render adapter was removed pending redesign. Its replacement must read ECS data through the public `ECS::World` query API and submit backend-neutral descriptions to `RenderWorld`. It must not create GPU resources or send device commands.

At frame start, `Rendering::Renderer` collects visible items, applies layer filtering and stable sort order, resolves their asset handles, and sends backend-neutral commands. Both back ends receive the same resolved draw data.

When an editor or asset tool replaces mesh, texture, shader, or material data, it must call the matching `Renderer::Invalidate` overload outside an active frame. The renderer waits for in-flight work and removes dependent cached GPU resources. The next visible draw uploads the current asset data again.
