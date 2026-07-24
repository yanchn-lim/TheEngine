#pragma once

#include "ecs_entity.hpp"
#include "ecs_component_pool.hpp"
#include "ecs_component_pool_interface.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>

namespace ECS
{
	using ComponentTypeID = std::size_t;

	namespace Detail
	{
		inline ComponentTypeID AllocateComponentTypeID() noexcept
		{
			static std::atomic<ComponentTypeID> nextId = 0;
			return nextId.fetch_add(1, std::memory_order_relaxed);
		}

		template<typename Component>
		inline ComponentTypeID GetComponentTypeIdImpl() noexcept
		{
			static ComponentTypeID typeId = AllocateComponentTypeID();
			return typeId;
		}
	}

	template<typename Component>
	inline ComponentTypeID GetComponentTypeId() noexcept
	{
		using PlainComponent = std::remove_cvref_t<Component>;
		return Detail::GetComponentTypeIdImpl<PlainComponent>();
	}

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
		ComponentPool<Component>& GetOrCreatePool()
		{
			const ComponentTypeID typeID = GetComponentTypeId<Component>();
			if (typeID >= _componentPools.size())
				_componentPools.resize(typeID + 1);

			if (!_componentPools[typeID])
			{
				_componentPools[typeID] = std::make_unique<ComponentPool<Component>>();
			}

			return *static_cast<ComponentPool<Component>*>(_componentPools[typeID].get());
		}

		template<typename Component>
		Component& AddComponent(Entity entity, const Component& component)
		{
			if (!IsEntityAlive(entity))
				throw std::runtime_error("Entity is not alive");

			auto& pool = GetOrCreatePool<Component>();
			return pool.AddComponent(entity, component);
		}
	};
}