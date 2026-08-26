#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ecs/ecs_entity.hpp"
#include "scene_asset_context.hpp"
#include "scene_load_error.hpp"

namespace ECS
{
	class World;
}

namespace Serialization
{
	class LSceneValue;
}

namespace Ludus
{
	class SceneComponentRegistry
	{
	public:
		// Component loaders must not depend on the order of other component loaders.
		using Loader = std::function<bool(
			const Serialization::LSceneValue&,
			const SceneAssetContext&,
			ECS::World&,
			ECS::Entity,
			std::vector<SceneLoadError>&)>;

		bool Register(std::string name, Loader loader);
		bool Load(
			std::string_view name,
			const Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			ECS::World& world,
			ECS::Entity entity,
			std::vector<SceneLoadError>& errors) const;

	private:
		std::unordered_map<std::string, Loader> _loaders;
	};

	void RegisterBuiltInSceneComponents(SceneComponentRegistry& registry);
}
