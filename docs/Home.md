# TheEngine Docs

This vault tracks engine architecture, implementation order, and system boundaries.

## Start Here

- [[Roadmap]]
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

Graphics backend work is active. The current checkpoint is finishing mesh support for both indexed and non-indexed drawing, then verifying the indexed path with a small manual render test.
