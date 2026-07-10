# System Boundaries

## Principle

Graphics and assets should be reusable backend services. They should not depend on the game object model, Scene, ECS, or E-CS.

## Dependency Direction

High-level systems depend on lower-level services:

```text
Game / App
Scene or ECS
Render Submission
Asset Manager
Graphics Backend
Platform / OpenGL
```

Lower-level services should not depend upward.

## Allowed Dependencies

Scene or ECS may depend on:

- Components
- Render submission systems
- Asset handles

Render submission may depend on:

- Scene/object queries or ECS queries
- Asset manager/resource lookup
- Graphics DrawCmd types

Renderer may depend on:

- Shader
- Texture2D
- Mesh
- Buffers
- DrawCmd
- Graphics state

Renderer should not depend on:

- Scene
- GameObject
- SpriteComponent
- ECS registry

Asset manager should not depend on:

- Scene
- GameObject
- Render submission behavior
- Gameplay logic

## Current Temporary Test Boundary

`ManualRenderTest` currently lives at the app/engine level. It is allowed to depend on both `Assets::AssetManager` and `Graphics::Renderer` because it is a temporary manual verification harness, not a reusable backend service.

It should not grow into the final render submission architecture. Future scene or ECS rendering should replace it with a dedicated submission layer that resolves asset handles before commands reach `Graphics::Renderer`.
