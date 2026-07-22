# System Boundaries

Scene and ECS code can depend on rendering descriptions and asset handles. It cannot depend on graphics devices or backend headers.

Rendering can depend on assets and graphics interfaces. It converts render intent into GPU commands.

Assets can depend on import data and file services. Asset registries own CPU data only and do not upload GPU resources.

Graphics interfaces cannot depend on a backend. The OpenGL and Vulkan implementations can depend on the shared interfaces and their native API.

The required direction is:

```text
scene and ECS -> rendering
editor -> scene, ECS, and rendering
rendering -> assets and graphics interfaces
assets -> import data and file services
OpenGL backend -> graphics interfaces
Vulkan backend -> graphics interfaces
```
