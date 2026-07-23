#include "ecs_def.hpp"
#include "ecs_world.hpp"

namespace ECS
{
	uint32_t World::CreateEntity()
	{
		uint32_t entityId = _entityCounter++;
		_entities.push_back(entityId);
		return entityId;
	}

	uint32_t World::GetEntityCount() const
	{
		return _entities.size();
	}

	void World::RemoveEntity(uint32_t entityId)
	{
		//find entity
		if (entityId == INVALID_ENTITY_ID)
			return;


	}
}