#include "debug/debug.hpp"
#include "mesh_registry.hpp"

namespace Ludus::Assets
{
	MeshHandle MeshRegistry::Create(const std::string& name, const MeshImportData& data)
	{
		// mesh names act as simple deduplication keys inside the registry
		if (data.surfaces.empty())
		{
			Ludus::Debug::LogError("MeshRegistry::Create : Mesh has no surfaces");
			return {};
		}

		for (const MeshSurface& surface : data.surfaces)
		{
			if (surface.vertices.empty() || surface.indices.empty())
			{
				Ludus::Debug::LogError("MeshRegistry::Create : Mesh surface has no geometry");
				return {};
			}
		}

		const auto it = _nameToHandle.find(name);
		if (it != _nameToHandle.end())
			return it->second;

		const MeshHandle handle{ _nextId++ };
		_nameToHandle[name] = handle;
		_meshes[handle.id] = MeshAsset{ data.surfaces, name };

		return handle;
	}

	const MeshAsset* MeshRegistry::Get(MeshHandle handle) const
	{
		// handles are lightweight ids while registry lookup is the ownership boundary
		if (!handle)
		{
			Ludus::Debug::LogError("MeshRegistry::Get : MeshHandle [", handle.id, "] is invalid");
			return nullptr;
		}

		const auto it = _meshes.find(handle.id);
		if (it == _meshes.end())
		{
			Ludus::Debug::LogError("MeshRegistry::Get : Could not find MeshHandle [", handle.id, "] in the registry.");
			return nullptr;
		}

		return &it->second;
	}

	bool MeshRegistry::SetSurfaceMaterial(
		MeshHandle mesh,
		std::string_view surface,
		MaterialHandle material)
	{
		const auto found = _meshes.find(mesh.id);
		if (found == _meshes.end() || !material)
			return false;

		for (MeshSurface& candidate : found->second.surfaces)
		{
			if (candidate.name == surface)
			{
				candidate.material = material;
				return true;
			}
		}

		return false;
	}

	void MeshRegistry::Clear()
	{
		// destroy all registry-owned CPU meshes and reset handle allocation
		_nextId = 1;
		_nameToHandle.clear();
		_meshes.clear();
	}
}
