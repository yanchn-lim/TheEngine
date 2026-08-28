#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ecs/ecs_world.hpp"
#include "scene_asset_context.hpp"
#include "serialization/lscene_value.hpp"

namespace Ludus
{
	struct SceneSystemDefinition
	{
		std::string id;
		bool enabled = true;
		Ludus::Serialization::LSceneValue config = Ludus::Serialization::LSceneValue::ObjectValue();
	};

	class Scene
	{
	public:
		struct EntityRecord
		{
			Ludus::ECS::Entity entity;
			std::string name;
		};

		// text ids remain stable when the ECS reuses entity slots.
		std::string CreateEntity(std::string name);
		Ludus::ECS::Entity RestoreEntity(std::string id, std::string name);
		// remove stable scene entities through Scene to keep their text IDs
		// synchronized with the World.
		bool RemoveEntity(std::string_view id);
		Ludus::ECS::Entity FindEntity(std::string_view id) const;
		std::string_view GetEntityName(std::string_view id) const;
		const std::unordered_map<std::string, EntityRecord>& GetEntities() const noexcept;

		void SetSerializationData(
			std::string name,
			Ludus::Serialization::LSceneValue assets,
			SceneAssetContext assetContext,
			std::vector<SceneSystemDefinition> systems = {});
		std::string_view GetName() const noexcept;
		const Ludus::Serialization::LSceneValue& GetAssetDeclarations() const noexcept;
		const SceneAssetContext& GetAssetContext() const noexcept;
		std::vector<SceneSystemDefinition>& GetSystems() noexcept;
		const std::vector<SceneSystemDefinition>& GetSystems() const noexcept;
		void SetSystems(std::vector<SceneSystemDefinition> systems);
		void Swap(Scene& other) noexcept;

		Ludus::ECS::World& GetWorld() noexcept;
		const Ludus::ECS::World& GetWorld() const noexcept;

		void FixedUpdate(double fixedDeltaTime);
		void Update();
	
	private:
		// Scene owns the World and maps serialized ids to its Entity handles.
		Ludus::ECS::World _world{};
		std::unordered_map<std::string, EntityRecord> _entities;
		std::string _name = "Untitled";
		// declarations preserve source data; the context stores resolved handles.
		Ludus::Serialization::LSceneValue _assets = Ludus::Serialization::LSceneValue::ObjectValue();
		SceneAssetContext _assetContext;
		// definitions preserve serialized state for save and editor operations.
		std::vector<SceneSystemDefinition> _systems;
	};
}
