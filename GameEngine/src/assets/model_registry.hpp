#pragma once

#include "asset_handle.hpp"
#include <unordered_map>

namespace Assets
{
	struct Model;

	struct ModelRegistry
	{
	public:
		ModelHandle Create(const std::string& name, std::vector<MeshHandle> handles);
		const Model* Get(ModelHandle handle) const;
		const Model* Get(const std::string& handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, ModelHandle> _nameToHandle;
		std::unordered_map<AssetId, Model> _models;
	};
}