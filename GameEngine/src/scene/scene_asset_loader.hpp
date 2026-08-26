#pragma once

#include <string_view>
#include <vector>

#include "scene_asset_context.hpp"
#include "scene_load_error.hpp"

namespace Assets
{
	class AssetManager;
}

namespace Serialization
{
	class LSceneValue;
}

namespace Ludus
{
	class SceneAssetLoader
	{
	public:
		static bool Load(
			const Serialization::LSceneValue& root,
			std::string_view sceneNamespace,
			Assets::AssetManager& assets,
			SceneAssetContext& context,
			std::vector<SceneLoadError>& errors);
	};
}
