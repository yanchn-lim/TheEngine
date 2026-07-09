# TheEngine Docs

This vault tracks engine architecture, implementation order, and system boundaries.

## Start Here

- [[Roadmap]]
- [[Next Steps TODO]]
- [[Architecture/System Boundaries]]
- [[Architecture/Graphics Backend]]
- [[Architecture/Asset Manager]]
- [[Architecture/Render Submission]]
- [[Canvases/Engine Systems.canvas|Engine Systems Canvas]]

## Current Direction

The engine should keep graphics and asset management reusable. They should not depend on GameObject, Scene, ECS, or any future entity-component system.

The immediate implementation goal is:

1. Finish a reliable graphics backend.
2. Build a reusable asset manager.
3. Integrate asset resolution with render submission.
4. Continue higher-level scene, gameplay, and ECS/E-CS work.

## Current Focus

Asset manager work is active. The current checkpoint is turning loaded multi-mesh model results into a proper runtime model asset with `ModelHandle` and `ModelRegistry`, then cleaning manual OBJ tests out of permanent startup flow.
