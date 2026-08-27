#pragma once

#include <string_view>

#include "asset_handle.hpp"
#include "asset_records.hpp"
#include "mesh_import_data.hpp"
namespace Ludus::Assets
{
	// owns CPU mesh records and deduplicates them by name
	class MeshRegistry
	{
	public:
		MeshHandle Create(const std::string& name, const MeshImportData& data);
		const MeshAsset* Get(MeshHandle handle) const;
		bool SetSurfaceMaterial(MeshHandle mesh, std::string_view surface, MaterialHandle material);
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, MeshHandle> _nameToHandle;
		std::unordered_map<AssetId, MeshAsset> _meshes;
	};
}
