#include "asset_manager.hpp"
#include "debug/debug.hpp"
#include "obj_importer.hpp"

namespace Ludus::Assets
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
		MeshImportData mesh;
		if (!_modelImporters.Import(path, mesh))
		{
			Ludus::Debug::LogError("AssetManager::LoadMesh : Failed to load mesh from ", path);
			return {};
		}

		return _meshes.Create(name, mesh);
	}

	MaterialHandle AssetManager::CreateMaterial(const std::string& name, ShaderHandle shader, TextureHandle texture, Ludus::Graphics::RenderState state)
	{
		if (!Get(shader) || !Get(texture))
		{
			Ludus::Debug::LogError("AssetManager::CreateMaterial : Could not resolve shader or texture fallback for material ", name);
			return MaterialHandle{};
		}

		return _materials.Create(name, shader, texture, state);
	}

	MeshHandle AssetManager::CreateMesh(const std::string& name, const MeshSurface& surface)
	{
		// keep procedural mesh data in the asset registry until RenderResourceManager uploads it
		MeshImportData mesh;
		mesh.surfaces.push_back(surface);
		return _meshes.Create(name, mesh);
	}

	bool AssetManager::SetSurfaceMaterial(
		MeshHandle mesh,
		std::string_view surface,
		MaterialHandle material)
	{
		return Get(material) && _meshes.SetSurfaceMaterial(mesh, surface, material);
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

	void AssetManager::Clear()
	{
		// clear dependent asset records before their referenced handles
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
