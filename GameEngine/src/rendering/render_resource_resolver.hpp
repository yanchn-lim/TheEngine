#pragma once

#include "render_item.hpp"
#include "assets/asset_manager.hpp"
#include "graphics/drawcmd.hpp"

namespace Rendering
{
	class RenderResourceResolver
	{
	public:
		explicit RenderResourceResolver(const Assets::AssetManager& assets);
		const Assets::ModelAsset* Resolve(Assets::ModelHandle handle) const;

		bool TryResolve(const RenderItem& item, Graphics::DrawCmd& outDrawCmd) const;

	private:
		const Assets::AssetManager& _assets;
	};
}