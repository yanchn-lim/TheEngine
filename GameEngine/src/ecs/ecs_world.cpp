#include "ecs_def.hpp"
#include "ecs_world.hpp"

namespace ECS
{
	Entity World::CreateEntity()
	{
		uint32_t entityId = 0;
		uint32_t generation = 0;

		//check free
		if (!_freeSlot.empty())
		{
			entityId = _freeSlot.back();
			_freeSlot.pop_back();
			_entities[entityId].alive = true;
			_entities[entityId].generation++;
		}
		else
		{
			entityId = static_cast<uint32_t>(_entities.size());
			_entities.push_back({ 1, true });
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
		if (!entity.IsValid())
			return;

		auto& slot = _entities[entity.id];
		if (slot.alive)
		{
			slot.alive = false;
			_freeSlot.push_back(entity.id);
			_entityAliveCount--;
		}
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		if (!entity.IsValid())
			return false;

		return _entities[entity.id].alive;
	}
}