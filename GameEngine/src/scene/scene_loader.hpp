#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "scene_component_registry.hpp"
#include "system_registry.hpp"

namespace Ludus::Assets
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
			Ludus::Assets::AssetManager& assets,
			const SceneComponentRegistry& components,
			const SystemRegistry& systems,
			std::vector<SceneLoadError>& errors);

		static bool LoadText(
			std::string_view source,
			const std::string& path,
			Scene& scene,
			Ludus::Assets::AssetManager& assets,
			const SceneComponentRegistry& components,
			const SystemRegistry& systems,
			std::vector<SceneLoadError>& errors);
	};
}
