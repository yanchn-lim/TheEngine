#pragma once

#include "asset_handle.hpp"
#include "graphics/mesh.hpp"

namespace Assets
{
	class MeshRegistry
	{
	public:
		//MeshHandle Load(const std::string& path);
		const Graphics::Mesh* Get(MeshHandle handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		//std::unordered_map<std::string, MeshHandle> _pathToHandle;
		std::unordered_map<AssetId, Graphics::Mesh> _meshes;
	};
}
