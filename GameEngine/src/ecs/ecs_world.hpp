#pragma once

#include "ecs_entity.hpp"
#include "ecs_component_pool_interface.hpp"

#include <cstdint>
#include <vector>
#include <memory>

namespace ECS
{
	struct EntitySlot
	{
		uint32_t generation;
		bool alive;
	};

	class World
	{
	private:
		std::vector<EntitySlot> _entitySlots{ EntitySlot{0,false} };
		std::vector<uint32_t> _freeSlots;
		std::vector<std::unique_ptr<IComponentPool>> _componentPools;
		uint32_t _entityAliveCount = 0;
	public:
		World() = default;
		~World() = default;

		Entity CreateEntity();
		uint32_t GetEntityCount() const;
		void RemoveEntity(Entity entity);
		bool IsEntityAlive(Entity entity) const;
	};
}