#pragma once

#include "ecs_entity.hpp"
#include "ecs_component_pool.hpp"
#include "ecs_component_pool_interface.hpp"
#include "ecs_component_view.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <type_traits>
#include <cstddef>
#include <stdexcept>

namespace ECS
{
	namespace Internal
	{
		using ComponentTypeID = std::size_t;

		inline ComponentTypeID AllocateComponentTypeID() noexcept
		{
			static std::atomic<ComponentTypeID> nextId = 0;
			return nextId.fetch_add(1, std::memory_order_relaxed);
		}

		template<typename Component>
		inline ComponentTypeID GetComponentTypeIdImpl() noexcept
		{
			static const ComponentTypeID typeId = AllocateComponentTypeID();
			return typeId;
		}

		template<typename Component>
		inline ComponentTypeID GetComponentTypeId() noexcept
		{
			return GetComponentTypeIdImpl<Component>();
		}

		struct EntitySlot
		{
			uint32_t generation;
			bool alive;
		};
	}

	class World
	{
	private:
		std::vector<Internal::EntitySlot> _entitySlots{ Internal::EntitySlot{0,false} };
		std::vector<uint32_t> _freeSlots;
		std::vector<std::unique_ptr<Internal::IComponentPool>> _componentPools;
		uint32_t _entityAliveCount = 0;

		template<typename Component>
		Internal::ComponentPool<Component>& GetOrCreatePool()
		{
			static_assert(std::is_same_v<Component, std::remove_cvref_t<Component>>,
				"Component type must not be const, volatile, or a reference");

			const Internal::ComponentTypeID typeID = Internal::GetComponentTypeId<Component>();
			if (typeID >= _componentPools.size())
				_componentPools.resize(typeID + 1);

			if (!_componentPools[typeID])
			{
				_componentPools[typeID] = std::make_unique<Internal::ComponentPool<Component>>();
			}

			return *static_cast<Internal::ComponentPool<Component>*>(_componentPools[typeID].get());
		}

		template<typename Component>
		Internal::ComponentPool<Component>* FindPool()
		{
			static_assert(std::is_same_v<Component, std::remove_cvref_t<Component>>,
				"Component type must not be const, volatile, or a reference");

			const Internal::ComponentTypeID typeID = Internal::GetComponentTypeId<Component>();
			if (typeID >= _componentPools.size() || !_componentPools[typeID])
				return nullptr;

			return static_cast<Internal::ComponentPool<Component>*>(_componentPools[typeID].get());
		}

		template<typename Component>
		const Internal::ComponentPool<Component>* FindPool() const
		{
			static_assert(std::is_same_v<Component, std::remove_cvref_t<Component>>,
				"Component type must not be const, volatile, or a reference");

			const Internal::ComponentTypeID typeID = Internal::GetComponentTypeId<Component>();
			if (typeID >= _componentPools.size() || !_componentPools[typeID])
				return nullptr;

			return static_cast<const Internal::ComponentPool<Component>*>(_componentPools[typeID].get());
		}
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

			auto& pool = GetOrCreatePool<Component>();
			return pool.AddComponent(entity, component);
		}

		template<typename Component>
		Component& GetComponent(Entity entity)
		{
			if (!IsEntityAlive(entity))
				throw std::runtime_error("Entity is not alive");

			auto* pool = FindPool<Component>();
			if (!pool)
				throw std::runtime_error("Entity does not have this component");

			return pool->GetComponent(entity);
		}

		template<typename Component>
		const Component& GetComponent(Entity entity) const
		{
			if (!IsEntityAlive(entity))
				throw std::runtime_error("Entity is not alive");

			const auto* pool = FindPool<Component>();
			if (!pool)
				throw std::runtime_error("Entity does not have this component");

			return pool->GetComponent(entity);
		}

		template<typename Component>
		Component* TryGetComponent(Entity entity)
		{
			if (!IsEntityAlive(entity))
				return nullptr;

			auto* pool = FindPool<Component>();
			return pool ? pool->TryGetComponent(entity) : nullptr;
		}

		template<typename Component>
		const Component* TryGetComponent(Entity entity) const
		{
			if (!IsEntityAlive(entity))
				return nullptr;

			const auto* pool = FindPool<Component>();
			return pool ? pool->TryGetComponent(entity) : nullptr;
		}

		template<typename Component>
		bool HasComponent(Entity entity) const
		{
			return TryGetComponent<Component>(entity) != nullptr;
		}

		template<typename Component>
		bool RemoveComponent(Entity entity)
		{
			if (!IsEntityAlive(entity))
				return false;

			auto* pool = FindPool<Component>();
			return pool && pool->RemoveIfPresent(entity);
		}
	
		template<typename Component>
		ComponentView<Component> GetComponentView()
		{
			//find pool of component
			auto* pool = FindPool<Component>();
			if (!pool)
				return {};

			return { pool->_denseEntities, pool->_denseComponents };
		}

		template<typename Component>
		ComponentView<const Component> GetComponentView() const
		{
			auto* pool = FindPool<Component>();
			if (!pool)
				return {};

			return { pool->_denseEntities, pool->_denseComponents };
		}
	};
}
