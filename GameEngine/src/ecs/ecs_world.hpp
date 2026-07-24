#pragma once

#include "ecs_entity.hpp"
#include "ecs_component_pool.hpp"
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

		template<typename Component>
		Component& AddComponent(Entity entity, const Component& component)
		{
			if (!IsEntityAlive(entity))
				throw std::runtime_error("Entity is not alive");

			uint32_t componentTypeId = GetComponentTypeId<Component>();
			if (componentTypeId >= _componentPools.size())
				_componentPools.resize(componentTypeId + 1);

			if (!_componentPools[componentTypeId])
				_componentPools[componentTypeId] = std::make_unique<ComponentPool<Component>>();

			auto* pool = static_cast<ComponentPool<Component>*>(_componentPools[componentTypeId].get());
			return pool->AddComponent(entity, component);
		}
	};
}