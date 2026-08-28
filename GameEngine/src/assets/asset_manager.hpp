#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "graphics/render_state.hpp"
#include "asset_handle.hpp"
#include "shader_registry.hpp"
#include "texture_registry.hpp"
#include "material_registry.hpp"
#include "mesh_registry.hpp"
#include "model_importer_registry.hpp"


namespace Ludus::Assets
{
    // owns CPU asset registries and exposes one typed-handle entry point to callers
    class AssetManager
	{
	public:
		TextureHandle LoadTexture(const std::string& path);
		ShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath,
			const std::string& vertexSpirvPath = {}, const std::string& fragmentSpirvPath = {});
		ShaderHandle LoadShaderResource(const std::string& path);
		MaterialHandle LoadMaterialResource(const std::string& path);
		MeshHandle LoadMesh(const std::string& name, const std::string& path);

		MaterialHandle CreateMaterial(
			const std::string& name,
			ShaderHandle shader,
			TextureHandle texture,
			Ludus::Graphics::RenderState state
		);

		MeshHandle CreateMesh(const std::string& name, const MeshSurface& surface);
		bool SetSurfaceMaterial(MeshHandle mesh, std::string_view surface, MaterialHandle material);

		const TextureAsset* Get(TextureHandle handle) const;
		const ShaderAsset* Get(ShaderHandle handle) const;
		const MaterialAsset* Get(MaterialHandle handle) const;
		const MaterialAsset* Get(const std::string& name) const;
		const MeshAsset* Get(MeshHandle handle) const;

		void Clear();

		AssetManager();

	private:
		ShaderRegistry _shaders;
		TextureRegistry _textures;
		MaterialRegistry _materials;
		MeshRegistry _meshes;
		ModelImporterRegistry _modelImporters;
		std::unordered_map<std::string, ShaderHandle> _shaderResources;
		std::unordered_map<std::string, MaterialHandle> _materialResources;

	};
}
