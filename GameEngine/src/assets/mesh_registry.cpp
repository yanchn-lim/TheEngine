#include "debug/debug.hpp"
#include "core/file_system.hpp"
#include "mesh_registry.hpp"

namespace Assets
{
	MeshHandle MeshRegistry::Create(const std::string& name, const ModelMeshData& data)
	{
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
		Graphics::Mesh mesh;
		Graphics::VertexLayout layout = Assets::CreateMeshVertexLayout();
		const uint32_t vertexCount = static_cast<uint32_t>(data.vertices.size());
		const uint32_t indexCount = static_cast<uint32_t>(data.indices.size());

		if (!mesh.Create(reinterpret_cast<const void*>(data.vertices.data()), vertexCount, layout, data.indices.data(), indexCount))
		{
			Debug::LogError("MeshRegistry::Create : Mesh creation failed");
			return MeshHandle();
		}
		_nameToHandle[name] = handle;
		_meshes[handle.id] = std::move(mesh);

		return handle;
	}

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
		_nameToHandle.clear();
		_meshes.clear();
	}
}