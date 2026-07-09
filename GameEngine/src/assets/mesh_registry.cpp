#include "debug/debug.hpp"
#include "core/file_system.hpp"
#include "mesh_registry.hpp"
#include "graphics/mesh_upload_data.hpp"

namespace Assets
{
	MeshHandle MeshRegistry::Create(const std::string& name, const MeshImportData& data)
	{
		// Mesh names act as simple deduplication keys inside the registry.
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

		Graphics::MeshUploadData meshUploadData;
		// Bridge asset-side mesh source data into the graphics backend upload format.
		meshUploadData.vertices = reinterpret_cast<const void*>(data.vertices.data());
		meshUploadData.vertexCount = vertexCount;
		meshUploadData.indices = data.indices.data();
		meshUploadData.indexCount = indexCount;
		meshUploadData.topology = data.topology;
		meshUploadData.layout = layout;

		if (!mesh.Create(meshUploadData))
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
		// Handles are lightweight ids; registry lookup is the ownership boundary.
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
		// Destroy all registry-owned GPU meshes and reset handle generation.
		_nextId = 1;
		_nameToHandle.clear();
		_meshes.clear();
	}
}
