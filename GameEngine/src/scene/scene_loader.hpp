#pragma once

#include <string>
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
	};
}
