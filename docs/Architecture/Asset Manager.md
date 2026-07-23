# Asset Manager

`Assets::AssetManager` owns backend-neutral asset records. It provides typed handles for meshes, models, textures, shaders, and materials.

- `MeshAsset` stores vertex bytes, index data, layout, topology, label, and version.
- `TextureAsset` stores decoded RGBA pixels, dimensions, sampler data, label, and version.
- `ShaderAsset` stores OpenGL GLSL and Vulkan SPIR-V variants.
- `MaterialAsset` stores shader and texture handles plus render state.
- `ModelAsset` stores the mesh handles produced by an importer.

Registries do not include OpenGL or Vulkan and do not own GPU objects. `Rendering::RenderResourceManager` resolves an asset handle on first use, creates the required device resources, caches them, and destroys them before device shutdown.

OBJ and primitive importers produce CPU mesh data. Texture import decodes files in the asset layer. Shader import reads both backend variants into one logical shader asset.
