#pragma once

#include <vector>

#include "graphics/render_state.hpp"
#include "asset_handle.hpp"
#include "shader_registry.hpp"
#include "texture_registry.hpp"
#include "material_registry.hpp"
#include "mesh_registry.hpp"
#include "model_importer_registry.hpp"
#include "model_asset.hpp"
#include "model_registry.hpp"


namespace Assets
{
    // owns CPU asset registries and exposes one typed-handle entry point to callers
    class AssetManager
	{
	public:
		TextureHandle LoadTexture(const std::string& path);
		ShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath,
			const std::string& vertexSpirvPath = {}, const std::string& fragmentSpirvPath = {});
		MeshHandle LoadMesh(const std::string& name, const std::string& path);
		ModelHandle LoadModel(const std::string& name, const std::string& path);

		MaterialHandle CreateMaterial(
			const std::string& name,
			ShaderHandle shader,
			TextureHandle texture,
			Graphics::RenderState state
		);

		MeshHandle CreateMesh(const std::string& name, const MeshImportData& data);

		const TextureAsset* Get(TextureHandle handle) const;
		const ShaderAsset* Get(ShaderHandle handle) const;
		const MaterialAsset* Get(MaterialHandle handle) const;
		const MaterialAsset* Get(const std::string& name) const;
		const MeshAsset* Get(MeshHandle handle) const;
		const ModelAsset* Get(ModelHandle handle) const;

		void Clear();

		AssetManager();

	private:
		ShaderRegistry _shaders;
		TextureRegistry _textures;
		MaterialRegistry _materials;
		MeshRegistry _meshes;
		ModelRegistry _models;
		ModelImporterRegistry _modelImporters;

	};
}
