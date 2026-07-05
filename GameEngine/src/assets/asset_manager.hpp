#pragma once

#include "graphics/render_state.hpp"
#include "asset_handle.hpp"
#include "shader_registry.hpp"
#include "texture_registry.hpp"
#include "material_registry.hpp"
#include "mesh_registry.hpp"
#include "model_loader.hpp"

namespace Assets
{
	class AssetManager
	{
	public:
		TextureHandle LoadTexture(const std::string& path);
		ShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
		MeshHandle LoadMesh(const std::string& name, const std::string& path);

		MaterialHandle CreateMaterial(
			const std::string& name,
			ShaderHandle shader,
			TextureHandle texture,
			Graphics::RenderState state
		);

		MeshHandle CreateMesh(const std::string& name, const MeshSourceData& data);

		const Graphics::Texture2D* Get(TextureHandle handle) const;
		const Graphics::Shader* Get(ShaderHandle handle) const;
		const Graphics::Material* Get(MaterialHandle handle) const;
		const Graphics::Material* Get(const std::string& name) const;
		const Graphics::Mesh* Get(MeshHandle handle) const;

		void Clear();

		AssetManager();

	private:
		const Graphics::Material* GetFallbackMaterial() const;

		ShaderRegistry _shaders;
		TextureRegistry _textures;
		MaterialRegistry _materials;
		MeshRegistry _meshes;
		ModelLoader _modelLoader;

		mutable Graphics::Material _fallbackMaterial;
		mutable bool _fallbackMaterialReady = false;
	};
}
