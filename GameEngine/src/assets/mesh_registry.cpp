#include "debug/debug.hpp"
#include "core/file_system.hpp"
#include "mesh_registry.hpp"

namespace Assets
{
	//MeshHandle MeshRegistry::Load(const std::string& path)
	//{
	//	//FileSystem::ReadTextFile();
	//}

	const Graphics::Mesh* MeshRegistry::Get(MeshHandle handle) const
	{
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
		_nextId = 1;
		_meshes.clear();
	}
}