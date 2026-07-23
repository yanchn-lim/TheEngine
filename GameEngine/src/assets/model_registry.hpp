#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "asset_handle.hpp"

namespace Assets
{
	struct ModelAsset;

	// owns model-to-mesh groupings and resolves model handles or names
	struct ModelRegistry
	{
	public:
		ModelHandle Create(const std::string& name, std::vector<MeshHandle> handles);
		const ModelAsset* Get(ModelHandle handle) const;
		const ModelAsset* Get(const std::string& handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, ModelHandle> _nameToHandle;
		std::unordered_map<AssetId, ModelAsset> _models;
	};
}
