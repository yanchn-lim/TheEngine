#include "debug/debug.hpp"
#include "mesh_registry.hpp"

namespace Assets
{
	MeshHandle MeshRegistry::Create(const std::string& name, const MeshImportData& data)
	{
		// mesh names act as simple deduplication keys inside the registry
		if (data.vertices.empty() || data.indices.empty())
		{
			Debug::LogError("MeshRegistry::Create : Mesh vertices or indices is empty");
			return MeshHandle();
		}
		const auto it = _nameToHandle.find(name);
		if (it != _nameToHandle.end())
		{
			return it->second;
		}

		MeshHandle handle{ _nextId++ };
		_nameToHandle[name] = handle;
		_meshes[handle.id] = MeshAsset{ data, name };

		return handle;
	}

	const MeshAsset* MeshRegistry::Get(MeshHandle handle) const
	{
		// handles are lightweight ids while registry lookup is the ownership boundary
		if (!handle)
		{
			Debug::LogError("MeshRegistry::Get : MeshHandle [", handle.id, "] is invalid");
			return nullptr;
		}

		const auto it = _meshes.find(handle.id);
		if (it == _meshes.end())
		{
			Debug::LogError("MeshRegistry::Get : Could not find MeshHandle [", handle.id, "] in the registry.");
			return nullptr;
		}

		return &it->second;
	}

	void MeshRegistry::Clear()
	{
		// destroy all registry-owned CPU meshes and reset handle allocation
		_nextId = 1;
		_nameToHandle.clear();
		_meshes.clear();
	}
}
