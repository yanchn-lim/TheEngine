#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ecs/ecs_world.hpp"
#include "scene_asset_context.hpp"
#include "scene_load_error.hpp"
#include "serialization/lscene_value.hpp"

namespace Ludus
{
	template<typename Component>
	struct SceneComponentCodec;

	class SceneComponentRegistry
	{
	public:
		using Loader = std::function<bool(
			const Serialization::LSceneValue&,
			const SceneAssetContext&,
			ECS::World&,
			ECS::Entity,
			std::vector<SceneLoadError>&)>;

		using Saver = std::function<bool(
			const SceneAssetContext&,
			const ECS::World&,
			ECS::Entity,
			Serialization::LSceneValue&,
			std::vector<std::string>&)>;

		using PresenceCheck = std::function<bool(const ECS::World&, ECS::Entity)>;

		template<typename Component>
		bool Register()
		{
			using Codec = SceneComponentCodec<Component>;

			return RegisterEntry(
				std::string(Codec::Name),
				[](const Serialization::LSceneValue& value,
					const SceneAssetContext& assets,
					ECS::World& world,
					ECS::Entity entity,
					std::vector<SceneLoadError>& errors)
				{
					Component component;
					if (!Codec::Load(value, assets, component, errors))
						return false;
					world.AddComponent(entity, component);
					return true;
				},
				[](const SceneAssetContext& assets,
					const ECS::World& world,
					ECS::Entity entity,
					Serialization::LSceneValue& output,
					std::vector<std::string>& errors)
				{
					const Component* component = world.TryGetComponent<Component>(entity);
					return component && Codec::Save(*component, assets, output, errors);
				},
				[](const ECS::World& world, ECS::Entity entity)
				{
					return world.HasComponent<Component>(entity);
				});
		}

		bool Load(
			std::string_view name,
			const Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			ECS::World& world,
			ECS::Entity entity,
			std::vector<SceneLoadError>& errors) const;

		bool SaveComponents(
			const SceneAssetContext& assets,
			const ECS::World& world,
			ECS::Entity entity,
			Serialization::LSceneValue::Object& output,
			std::vector<std::string>& errors) const;

	private:
		struct Entry
		{
			std::string name;
			Loader load;
			Saver save;
			PresenceCheck has;
		};

		bool RegisterEntry(
			std::string name,
			Loader loader,
			Saver saver,
			PresenceCheck has);

		std::vector<Entry> _entries;
		std::unordered_map<std::string, size_t> _indices;
	};

	void RegisterBuiltInSceneComponents(SceneComponentRegistry& registry);
}
