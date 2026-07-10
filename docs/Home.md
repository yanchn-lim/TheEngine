# TheEngine Docs

This vault tracks engine architecture, implementation order, and system boundaries.

## Start Here

- [[Roadmap]]
- [[Architecture/System Boundaries]]
- [[Architecture/Graphics Backend]]
- [[Architecture/Asset Manager]]
- [[Architecture/Render Submission]]
- [[To Do/Render Resource Resolver|Render Resource Resolver To-Do]]
- [[Canvases/Engine Systems.canvas|Engine Systems Canvas]]

## Current Direction

The engine should keep graphics and asset management reusable. They should not depend on GameObject, Scene, ECS, or any future entity-component system.

The immediate implementation goal is:

1. Finish a reliable graphics backend.
2. Build a reusable asset manager.
3. Integrate asset resolution with render submission.
4. Continue higher-level scene, gameplay, and ECS/E-CS work.

## Current Focus

Graphics and asset integration work is active. The current checkpoint is defining a clean render resource access boundary so draw submission can resolve asset handles without making `Graphics::Renderer` depend on `Assets::AssetManager`, Scene, GameObject, or ECS.
