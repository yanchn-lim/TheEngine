#include "asset_manager.hpp"
#include "debug/debug.hpp"

namespace Assets
{
	TextureHandle AssetManager::LoadTexture(const std::string& path)
	{
		return _textures.Load(path);
	}

	ShaderHandle AssetManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		return _shaders.Load(vertexPath, fragmentPath);
	}

	MaterialHandle AssetManager::CreateMaterial(const std::string& name, ShaderHandle shader, TextureHandle texture, Graphics::RenderState state)
	{
		const Graphics::Shader* shaderPtr = Get(shader);
		const Graphics::Texture2D* texturePtr = Get(texture);

		if (!shaderPtr || !texturePtr)
		{
			Debug::LogError("AssetManager::CreateMaterial : Could not resolve shader or texture fallback for material ", name);
			return MaterialHandle{};
		}

		return _materials.Create(name, shaderPtr, texturePtr, state);
	}

	MeshHandle AssetManager::CreateMesh(const std::string& name, const ModelMeshData& data)
	{

		return _meshes.Create(name, data);
	}

	const Graphics::Texture2D* AssetManager::Get(TextureHandle handle) const 
	{
		return _textures.Get(handle);
	}

	const Graphics::Shader* AssetManager::Get(ShaderHandle handle) const
	{
		return _shaders.Get(handle);
	}

	const Graphics::Material* AssetManager::Get(MaterialHandle handle) const
	{
		const Graphics::Material* material = _materials.Get(handle);
		if (!material)
		{
			Debug::LogError("AssetManager::Get : Returning fallback material for MaterialHandle [", handle.id, "]");
			return GetFallbackMaterial();
		}

		return material;
	}

	const Graphics::Material* AssetManager::Get(const std::string& name) const
	{
		const Graphics::Material* material = _materials.Get(name);
		if (!material)
		{
			Debug::LogError("AssetManager::Get : Returning fallback material for material name ", name);
			return GetFallbackMaterial();
		}

		return material;
	}

	const Graphics::Mesh* AssetManager::Get(MeshHandle handle) const
	{
		return _meshes.Get(handle);
	}

	const Graphics::Material* AssetManager::GetFallbackMaterial() const
	{
		if (_fallbackMaterialReady)
			return &_fallbackMaterial;

		const Graphics::Shader* shader = _shaders.Get(ShaderHandle{});
		const Graphics::Texture2D* texture = _textures.Get(TextureHandle{});

		if (!shader || !texture)
		{
			Debug::LogError("AssetManager::GetFallbackMaterial : Could not create fallback material");
			return nullptr;
		}

		_fallbackMaterial.shader = shader;
		_fallbackMaterial.texture = texture;
		_fallbackMaterial.state = Graphics::RenderState{};
		_fallbackMaterialReady = true;

		return &_fallbackMaterial;
	}

	void AssetManager::Clear()
	{
		_fallbackMaterial = Graphics::Material{};
		_fallbackMaterialReady = false;
		_meshes.Clear();
		_materials.Clear();
		_shaders.Clear();
		_textures.Clear();
	}

}
