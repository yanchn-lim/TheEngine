#pragma once

#include "ecs_component_pool_interface.hpp"
#include "ecs_entity.hpp"

#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ECS::Internal
{
	constexpr uint32_t INVALID_COMPONENT_INDEX = UINT32_MAX;

	template<typename Component>
	class ComponentPool final : public IComponentPool
	{
		friend class ECS::World;

	private:
		std::vector<Entity> _denseEntities;
		std::vector<Component> _denseComponents;
		std::vector<uint32_t> _sparseIndices; //same size as the maximum entity id, maps entity id to dense index

		bool _validEntity(Entity entity) const
		{
			return entity.IsValid() && entity.id < _sparseIndices.size();
		}

	public:
		ComponentPool() = default;

	private:
		Component& AddComponent(Entity entity, const Component& component)
		{
			if (!entity.IsValid())
				throw std::runtime_error("Invalid entity");

			if (Contains(entity))
				throw std::runtime_error("Entity already has this component");

			if (_denseComponents.size() >= INVALID_COMPONENT_INDEX)
				throw std::length_error("Component pool is full");

			const uint32_t denseIndex = static_cast<uint32_t>(_denseComponents.size());
			const std::size_t requiredSparseSize = static_cast<std::size_t>(entity.id) + 1;

			if (_sparseIndices.size() < requiredSparseSize)
				_sparseIndices.resize(requiredSparseSize, INVALID_COMPONENT_INDEX);
			_denseEntities.push_back(entity);

			try
			{
				_denseComponents.push_back(component);
			}
			catch (...)
			{
				_denseEntities.pop_back();
				throw;
			}

			_sparseIndices[entity.id] = denseIndex;
			return _denseComponents.back();
		}

		Component& GetComponent(Entity entity)
		{
			Component* component = TryGetComponent(entity);
			if (!component)
				throw std::runtime_error("Entity does not have this component");

			return *component;
		}

		const Component& GetComponent(Entity entity) const
		{
			const Component* component = TryGetComponent(entity);
			if (!component)
				throw std::runtime_error("Entity does not have this component");

			return *component;
		}

		Component* TryGetComponent(Entity entity)
		{
			if (!Contains(entity))
				return nullptr;

			return &_denseComponents[_sparseIndices[entity.id]];
		}

		const Component* TryGetComponent(Entity entity) const
		{
			if (!Contains(entity))
				return nullptr;

			return &_denseComponents[_sparseIndices[entity.id]];
		}

		bool RemoveIfPresent(Entity entity) override
		{
			if (!Contains(entity))
				return false;

			const uint32_t removedIndex = static_cast<uint32_t>(_sparseIndices[entity.id]);
			const uint32_t lastIndex = static_cast<uint32_t>(_denseComponents.size() - 1);

			if (removedIndex != lastIndex)
			{
				//swap
				std::swap(_denseComponents[removedIndex], _denseComponents[lastIndex]);
				std::swap(_denseEntities[removedIndex], _denseEntities[lastIndex]);
				const Entity& movedEntity = _denseEntities[removedIndex];
				_sparseIndices[movedEntity.id] = removedIndex;
			}

			_denseComponents.pop_back();
			_denseEntities.pop_back();
			_sparseIndices[entity.id] = INVALID_COMPONENT_INDEX;
			return true;
		}

		std::size_t GetSize() const noexcept override
		{
			return _denseComponents.size();
		}

		std::span<const Entity> GetEntities() const noexcept override
		{
			return _denseEntities;
		}

		bool Contains(Entity entity) const override
		{
			if (!_validEntity(entity))
				return false;

			const uint32_t denseIndex = _sparseIndices[entity.id];
			if (denseIndex == INVALID_COMPONENT_INDEX || denseIndex >= _denseEntities.size())
				return false;

			return _denseEntities[denseIndex] == entity;
		}
	};
}
