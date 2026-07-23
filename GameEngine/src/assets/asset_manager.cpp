#include "asset_manager.hpp"
#include "debug/debug.hpp"
#include "obj_importer.hpp"

namespace Assets
{
	TextureHandle AssetManager::LoadTexture(const std::string& path)
	{
		// decode and register CPU texture data
		return _textures.Load(path);
	}

	ShaderHandle AssetManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath,
		const std::string& vertexSpirvPath, const std::string& fragmentSpirvPath)
	{
		// store backend shader variants and deduplicate them by source paths
		return _shaders.Load(vertexPath, fragmentPath, vertexSpirvPath, fragmentSpirvPath);
	}

	MeshHandle AssetManager::LoadMesh(const std::string& name, const std::string& path)
	{
		// imported model data is temporary; the registry keeps the first CPU mesh record
		ModelImportData collection;
		if (!_modelImporters.Import(path, collection))
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
		ModelImportData collection;
		if (!_modelImporters.Import(path, collection))
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
		if (!Get(shader) || !Get(texture))
		{
			Debug::LogError("AssetManager::CreateMaterial : Could not resolve shader or texture fallback for material ", name);
			return MaterialHandle{};
		}

		return _materials.Create(name, shader, texture, state);
	}

	MeshHandle AssetManager::CreateMesh(const std::string& name, const MeshImportData& data)
	{
		// keep procedural mesh data in the asset registry until RenderResourceManager uploads it
		return _meshes.Create(name, data);
	}

	const TextureAsset* AssetManager::Get(TextureHandle handle) const
	{
		return _textures.Get(handle);
	}

	const ShaderAsset* AssetManager::Get(ShaderHandle handle) const
	{
		return _shaders.Get(handle);
	}

	const MaterialAsset* AssetManager::Get(MaterialHandle handle) const
	{
		return _materials.Get(handle);
	}

	const MaterialAsset* AssetManager::Get(const std::string& name) const
	{
		return _materials.Get(name);
	}

	const MeshAsset* AssetManager::Get(MeshHandle handle) const
	{
		return _meshes.Get(handle);
	}

	const ModelAsset* AssetManager::Get(ModelHandle handle) const
	{
		return _models.Get(handle);
	}

	void AssetManager::Clear()
	{
		// clear dependent asset records before their referenced handles
		_models.Clear();
		_meshes.Clear();
		_materials.Clear();
		_shaders.Clear();
		_textures.Clear();
	}

	AssetManager::AssetManager()
	{
		// register format importers here so callers only talk to AssetManager
		_modelImporters.RegisterImporter(std::make_unique<ObjImporter>());
	}

}
