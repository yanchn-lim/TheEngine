#pragma once

#include "asset_handle.hpp"
#include "graphics/material.hpp"

namespace Assets
{
	class MaterialRegistry
	{
	public:
		MaterialHandle Create(const std::string& name, const Graphics::Shader* shader, const Graphics::Texture2D* texture, Graphics::RenderState state);
		const Graphics::Material* Get(MaterialHandle handle) const;
		const Graphics::Material* Get(const std::string& handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, MaterialHandle> _nameToHandle;
		std::unordered_map<AssetId, Graphics::Material> _materials;
	};
}