#include "ecs_def.hpp"
#include "ecs_world.hpp"

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
			_entitySlots[entityId].alive = true;
			_entitySlots[entityId].generation++;
			generation = _entitySlots[entityId].generation;
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
			slot.alive = false;
			_freeSlots.push_back(entity.id);
			_entityAliveCount--;

			//remove components
			for(auto& pool : _componentPools)
			{
				pool->RemoveIfPresent(entity);
			}
		}
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		if (!entity.IsValid())
			return false;

		if (entity.id >= _entitySlots.size())
			return false;

		const EntitySlot& slot = _entitySlots[entity.id];

		return slot.alive && slot.generation == entity.generation;
	}
}