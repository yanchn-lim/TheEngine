#pragma once

#include "asset_handle.hpp"
#include "asset_records.hpp"

namespace Ludus::Assets
{
	// decodes texture files and owns their CPU pixel records
	class TextureRegistry
	{
	public:
		TextureHandle Load(const std::string& path);
		TextureHandle CreateSolidColor(
			const std::string& name,
			unsigned char red,
			unsigned char green,
			unsigned char blue,
			unsigned char alpha);
		const TextureAsset* Get(TextureHandle handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, TextureHandle> _pathToHandle;
		std::unordered_map<AssetId, TextureAsset> _textures;
	};
}
