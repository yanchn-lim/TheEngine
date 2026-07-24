#pragma once

#include "ecs_component_pool_interface.hpp"
#include "ecs_entity.hpp"

#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ECS
{
	constexpr uint32_t INVALID_COMPONENT_INDEX = UINT32_MAX;

	template<typename Component>
	class ComponentPool : public IComponentPool
	{
	private:
		std::vector<Entity> _denseEntities;
		std::vector<Component> _denseComponents;
		std::vector<uint32_t> _sparseIndices; //same size as the maximum entity id, maps entity id to dense index

		bool _validEntity(const Entity& entity) const
		{
			return entity.IsValid() && entity.id < _sparseIndices.size();
		}

	public:
		ComponentPool() = default;

		Component& AddComponent(Entity entity, const Component& component)
		{
			//check entity validity
			if (!entity.IsValid())
				throw std::runtime_error("Invalid entity");

			if(Contains(entity))
				return GetComponent(entity);

			uint32_t denseIndex = _denseComponents.size();
			//should be same index for both
			_denseComponents.push_back(component);
			_denseEntities.push_back(entity);

			//resize sparse indices if needed
			_sparseIndices.resize(std::max(_sparseIndices.size(), static_cast<size_t>(entity.id + 1)), INVALID_COMPONENT_INDEX);
			_sparseIndices[entity.id] = denseIndex;
			return _denseComponents.back();
		}

		Component& GetComponent(Entity entity)
		{
			if (!_validEntity(entity))
				throw std::runtime_error("Invalid entity");

			if(!Contains(entity))
				throw std::runtime_error("Entity does not have this component");

			uint32_t denseIndex = _sparseIndices[entity.id];
			if (denseIndex >= _denseComponents.size())
				throw std::runtime_error("Invalid component index (index >= size)");

			if(denseIndex == INVALID_COMPONENT_INDEX)
				throw std::runtime_error("Entity does not have this component");

			return _denseComponents[denseIndex];
		}

		void RemoveIfPresent(Entity entity) override
		{
			if (!Contains(entity))
				return;

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