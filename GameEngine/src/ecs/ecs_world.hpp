#pragma once

#include "ecs_entity.hpp"

#include <cstdint>
#include <vector>

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