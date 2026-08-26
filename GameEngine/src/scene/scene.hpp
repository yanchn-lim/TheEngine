#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "ecs/ecs_world.hpp"

namespace Ludus
{
	class Scene
	{
	public:
		ECS::Entity CreateEntity(std::string id, std::string name);
		// Remove stable scene entities through Scene to keep their text IDs synchronized.
		bool RemoveEntity(std::string_view id);
		ECS::Entity FindEntity(std::string_view id) const;
		std::string_view GetEntityName(std::string_view id) const;
		void Swap(Scene& other) noexcept;

		ECS::World& GetWorld() noexcept;
		const ECS::World& GetWorld() const noexcept;

		void FixedUpdate(double fixedDeltaTime);
		void Update();
	
	private:
		struct EntityRecord
		{
			ECS::Entity entity;
			std::string name;
		};

		ECS::World _world{};
		std::unordered_map<std::string, EntityRecord> _entities;
	};
}
