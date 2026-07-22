#pragma once

#include "asset_handle.hpp"
#include "asset_records.hpp"

namespace Assets
{
	// decodes texture files and owns their CPU pixel records
	class TextureRegistry
	{
	public:
		TextureHandle Load(const std::string& path);
		const TextureAsset* Get(TextureHandle handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, TextureHandle> _pathToHandle;
		std::unordered_map<AssetId, TextureAsset> _textures;
	};
}
