#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
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
			const Ludus::Serialization::LSceneValue&,
			const SceneAssetContext&,
			Ludus::ECS::World&,
			Ludus::ECS::Entity,
			std::vector<SceneLoadError>&)>;

		using Saver = std::function<bool(
			const SceneAssetContext&,
			const Ludus::ECS::World&,
			Ludus::ECS::Entity,
			Ludus::Serialization::LSceneValue&,
			std::vector<std::string>&)>;

		using PresenceCheck = std::function<bool(const Ludus::ECS::World&, Ludus::ECS::Entity)>;
		using Updater = std::function<bool(
			const Ludus::Serialization::LSceneValue&,
			const SceneAssetContext&,
			Ludus::ECS::World&,
			Ludus::ECS::Entity,
			std::vector<SceneLoadError>&)>;

		template<typename Component>
		bool Register()
		{
			using Codec = SceneComponentCodec<Component>;
			static_assert(
				std::is_move_assignable_v<Component>,
				"Editable scene components must be move assignable");

			return RegisterEntry(
				std::string(Codec::Name),
				[](const Ludus::Serialization::LSceneValue& value,
					const SceneAssetContext& assets,
					Ludus::ECS::World& world,
					Ludus::ECS::Entity entity,
					std::vector<SceneLoadError>& errors)
				{
					Component component;
					if (!Codec::Load(value, assets, component, errors))
						return false;
					world.AddComponent(entity, component);
					return true;
				},
				[](const SceneAssetContext& assets,
					const Ludus::ECS::World& world,
					Ludus::ECS::Entity entity,
					Ludus::Serialization::LSceneValue& output,
					std::vector<std::string>& errors)
				{
					const Component* component = world.TryGetComponent<Component>(entity);
					return component && Codec::Save(*component, assets, output, errors);
				},
				[](const Ludus::ECS::World& world, Ludus::ECS::Entity entity)
				{
					return world.HasComponent<Component>(entity);
				},
				[](const Ludus::Serialization::LSceneValue& value,
					const SceneAssetContext& assets,
					Ludus::ECS::World& world,
					Ludus::ECS::Entity entity,
					std::vector<SceneLoadError>& errors)
				{
					Component* existing = world.TryGetComponent<Component>(entity);
					if (!existing)
					{
						errors.push_back({
							"entity does not have component '" +
							std::string(Codec::Name) + "'",
							value.GetLocation() });
						return false;
					}

					Component updated;
					if (!Codec::Load(value, assets, updated, errors))
						return false;
					*existing = std::move(updated);
					return true;
				});
		}

		bool Load(
			std::string_view name,
			const Ludus::Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			Ludus::ECS::World& world,
			Ludus::ECS::Entity entity,
			std::vector<SceneLoadError>& errors) const;

		bool SaveComponents(
			const SceneAssetContext& assets,
			const Ludus::ECS::World& world,
			Ludus::ECS::Entity entity,
			Ludus::Serialization::LSceneValue::Object& output,
			std::vector<std::string>& errors) const;

		bool Update(
			std::string_view name,
			const Ludus::Serialization::LSceneValue& value,
			const SceneAssetContext& assets,
			Ludus::ECS::World& world,
			Ludus::ECS::Entity entity,
			std::vector<SceneLoadError>& errors) const;

	private:
		struct Entry
		{
			std::string name;
			Loader load;
			Saver save;
			PresenceCheck has;
			Updater update;
		};

		bool RegisterEntry(
			std::string name,
			Loader loader,
			Saver saver,
			PresenceCheck has,
			Updater updater);

		std::vector<Entry> _entries;
		std::unordered_map<std::string, size_t> _indices;
	};

	void RegisterBuiltInSceneComponents(SceneComponentRegistry& registry);
}
