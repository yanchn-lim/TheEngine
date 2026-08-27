#pragma once

#include "asset_handle.hpp"
#include "asset_records.hpp"

namespace Ludus::Assets
{
	// owns material descriptions and resolves their typed handles
	class MaterialRegistry
	{
	public:
		MaterialHandle Create(const std::string& name, ShaderHandle shader, TextureHandle texture, Ludus::Graphics::RenderState state);
		const MaterialAsset* Get(MaterialHandle handle) const;
		const MaterialAsset* Get(const std::string& handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, MaterialHandle> _nameToHandle;
		std::unordered_map<AssetId, MaterialAsset> _materials;
	};
}
