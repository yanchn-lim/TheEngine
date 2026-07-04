#pragma once

#include "asset_handle.hpp"
#include "graphics/texture2d.hpp"

namespace Assets
{
	class TextureRegistry
	{
	public:
		TextureHandle Load(const std::string& path);
		const Graphics::Texture2D* Get(TextureHandle handle) const;
		void Clear();

	private:
		const Graphics::Texture2D* GetFallback() const;

		AssetId _nextId = 1;

		std::unordered_map<std::string, TextureHandle> _pathToHandle;
		std::unordered_map<AssetId, Graphics::Texture2D> _textures;

		mutable Graphics::Texture2D _fallbackTexture;
		mutable bool _fallbackTextureReady = false;
	};
}
