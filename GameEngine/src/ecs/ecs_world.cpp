#include "ecs_world.hpp"

#include <limits>

namespace ECS
{
	Entity World::CreateEntity()
	{
		uint32_t entityId = 0;
		uint32_t generation = 0;

		//check free
		if (!_freeSlots.empty())
		{
			entityId = _freeSlots.back();
			_freeSlots.pop_back();

			Internal::EntitySlot& slot = _entitySlots[entityId];

			if (slot.generation == std::numeric_limits<uint32_t>::max())
			{
				slot.generation = 1;
			}
			else
			{
				++slot.generation;
			}

			slot.alive = true;
			generation = slot.generation;
		}
		else
		{
			entityId = static_cast<uint32_t>(_entitySlots.size());
			generation = 1;
			_entitySlots.push_back({ generation, true });
		}

		_entityAliveCount++;
		return Entity{ entityId, generation };
	}

	uint32_t World::GetEntityCount() const
	{
		return _entityAliveCount;
	}

	void World::RemoveEntity(Entity entity)
	{
		//find entity
		if (!IsEntityAlive(entity))
			return;

		auto& slot = _entitySlots[entity.id];
		if (slot.alive)
		{
			//remove components
			for(auto& pool : _componentPools)
			{
				if(pool)
					pool->RemoveIfPresent(entity);
			}

			slot.alive = false;
			_freeSlots.push_back(entity.id);
			_entityAliveCount--;

		}
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		if (!entity.IsValid())
			return false;

		if (entity.id >= _entitySlots.size())
			return false;

		const Internal::EntitySlot& slot = _entitySlots[entity.id];

		return slot.alive && slot.generation == entity.generation;
	}
}
