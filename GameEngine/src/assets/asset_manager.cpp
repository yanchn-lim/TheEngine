#include "asset_manager.hpp"
#include "debug/debug.hpp"
#include "obj_model_importer.hpp"

namespace Assets
{
	TextureHandle AssetManager::LoadTexture(const std::string& path)
	{
		// Delegate file loading and fallback ownership to the texture registry.
		return _textures.Load(path);
	}

	ShaderHandle AssetManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		// Shader registry owns compiled programs and deduplicates by source paths.
		return _shaders.Load(vertexPath, fragmentPath);
	}

	MeshHandle AssetManager::LoadMesh(const std::string& name, const std::string& path)
	{
		// Imported model data is temporary; only the created GPU mesh is kept.
		MeshSourceCollection collection;
		if (!_modelLoader.Load(path, collection))
		{
			Debug::LogError("AssetManager::LoadMesh : Failed to load mesh from ", path);
			return MeshHandle();
		}

		if (collection.meshes.empty())
		{
			Debug::LogError("AssetManager::LoadMesh : No meshes found in ", path);
			return MeshHandle();
		}

		return _meshes.Create(name, collection.meshes[0]);
	}

	ModelHandle AssetManager::LoadModel(const std::string& name, const std::string& path)
	{
		MeshSourceCollection collection;
		if (!_modelLoader.Load(path, collection))
		{
			Debug::LogError("AssetManager::LoadModel : Failed to load mesh from ", path);
			return ModelHandle();
		}

		if (collection.meshes.empty())
		{
			Debug::LogError("AssetManager::LoadModel : No meshes found in ", path);
			return ModelHandle();
		}

		std::vector<MeshHandle> meshHandles{};
		for (size_t i = 0; i < collection.meshes.size(); ++i)
		{
			const std::string meshName = name + "_" + std::to_string(i);
			meshHandles.push_back(_meshes.Create(meshName, collection.meshes[i]));
		}

		return _models.Create(name, std::move(meshHandles));
	}

	MaterialHandle AssetManager::CreateMaterial(const std::string& name, ShaderHandle shader, TextureHandle texture, Graphics::RenderState state)
	{
		// Resolve handles up front so materials can hold renderer-facing pointers.
		const Graphics::Shader* shaderPtr = Get(shader);
		const Graphics::Texture2D* texturePtr = Get(texture);

		if (!shaderPtr || !texturePtr)
		{
			Debug::LogError("AssetManager::CreateMaterial : Could not resolve shader or texture fallback for material ", name);
			return MaterialHandle{};
		}

		return _materials.Create(name, shaderPtr, texturePtr, state);
	}

	MeshHandle AssetManager::CreateMesh(const std::string& name, const MeshSourceData& data)
	{
		// Procedural/source mesh data is converted into a registry-owned Graphics::Mesh.
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

	const Model* AssetManager::Get(ModelHandle handle) const
	{
		return _models.Get(handle);
	}


	const Graphics::Material* AssetManager::GetFallbackMaterial() const
	{
		// Build the fallback lazily so shader/texture fallback resources exist first.
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
		// Clear dependent resources before shader/texture registries invalidate pointers.
		_fallbackMaterial = Graphics::Material{};
		_fallbackMaterialReady = false;
		_meshes.Clear();
		_materials.Clear();
		_shaders.Clear();
		_textures.Clear();
	}

	AssetManager::AssetManager()
	{
		// Register format importers here so callers only talk to AssetManager.
		_modelLoader.RegisterImporter(std::make_unique<ObjModelImporter>());
	}

}
