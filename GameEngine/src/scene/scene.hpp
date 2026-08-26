#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "ecs/ecs_world.hpp"
#include "scene_asset_context.hpp"
#include "serialization/lscene_value.hpp"

namespace Ludus
{
	class Scene
	{
	public:
		struct EntityRecord
		{
			ECS::Entity entity;
			std::string name;
		};

		ECS::Entity CreateEntity(std::string id, std::string name);
		// Remove stable scene entities through Scene to keep their text IDs synchronized.
		bool RemoveEntity(std::string_view id);
		ECS::Entity FindEntity(std::string_view id) const;
		std::string_view GetEntityName(std::string_view id) const;
		const std::unordered_map<std::string, EntityRecord>& GetEntities() const noexcept;

		void SetSerializationData(
			std::string name,
			Serialization::LSceneValue assets,
			SceneAssetContext assetContext);
		std::string_view GetName() const noexcept;
		const Serialization::LSceneValue& GetAssetDeclarations() const noexcept;
		const SceneAssetContext& GetAssetContext() const noexcept;
		void Swap(Scene& other) noexcept;

		ECS::World& GetWorld() noexcept;
		const ECS::World& GetWorld() const noexcept;

		void FixedUpdate(double fixedDeltaTime);
		void Update();
	
	private:
		ECS::World _world{};
		std::unordered_map<std::string, EntityRecord> _entities;
		std::string _name = "Untitled";
		Serialization::LSceneValue _assets = Serialization::LSceneValue::ObjectValue();
		SceneAssetContext _assetContext;
	};
}
