#pragma once

#include "asset_handle.hpp"
#include "mesh_import_data.hpp"
#include "graphics/mesh.hpp"
namespace Assets
{
	class MeshRegistry
	{
	public:
		MeshHandle Create(const std::string& name, const MeshImportData& data);
		const Graphics::Mesh* Get(MeshHandle handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, MeshHandle> _nameToHandle;
		std::unordered_map<AssetId, Graphics::Mesh> _meshes;
	};
}
