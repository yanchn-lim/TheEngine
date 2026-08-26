#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "scene_component_registry.hpp"

namespace Assets
{
	class AssetManager;
}

namespace Ludus
{
	class Scene;

	class SceneLoader
	{
	public:
		static bool Load(
			const std::string& path,
			Scene& scene,
			Assets::AssetManager& assets,
			const SceneComponentRegistry& components,
			std::vector<SceneLoadError>& errors);

		static bool LoadText(
			std::string_view source,
			const std::string& path,
			Scene& scene,
			Assets::AssetManager& assets,
			const SceneComponentRegistry& components,
			std::vector<SceneLoadError>& errors);
	};
}
