#pragma once

#include "graphics/render_state.hpp"
#include "asset_handle.hpp"
#include "shader_registry.hpp"
#include "texture_registry.hpp"
#include "material_registry.hpp"

namespace Assets
{
	class AssetManager
	{
	public:
		TextureHandle LoadTexture(const std::string& path);
		ShaderHandle LoadShader(const std::string& vertexPath, const std::string& fragmentPath);

		MaterialHandle CreateMaterial(
			const std::string& name,
			ShaderHandle shader,
			TextureHandle texture,
			Graphics::RenderState state
		);

		const Graphics::Texture2D* Get(TextureHandle handle) const;
		const Graphics::Shader* Get(ShaderHandle handle) const;
		const Graphics::Material* Get(MaterialHandle handle) const;
		const Graphics::Material* Get(const std::string& name) const;

		void Clear();

	private:
		const Graphics::Material* GetFallbackMaterial() const;

		ShaderRegistry _shaders;
		TextureRegistry _textures;
		MaterialRegistry _materials;

		mutable Graphics::Material _fallbackMaterial;
		mutable bool _fallbackMaterialReady = false;
	};
}
