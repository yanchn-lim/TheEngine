#pragma once

#include "ecs_component_pool_interface.hpp"

namespace ECS
{
	template<typename Component>
	class ComponentPool : public IComponentPool
	{
	private:
		std::vector<Entity> _denseEntities;
		std::vector<Component> _denseComponents;
		std::vector<uint32_t> _sparseIndices;

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

			uint32_t _denseIndex = _denseComponents.size();
			//should be same index for both
			_denseComponents.push_back(component);
			_denseEntities.push_back(entity);

			//resize sparse indices if needed
			_sparseIndices.resize(std::max(_sparseIndices.size(), static_cast<size_t>(entity.id + 1)), INVALID_COMPONENT_INDEX);
			_sparseIndices[entity.id] = _denseIndex;
		}

		Component& GetComponent(Entity entity)
		{
			if (!_validEntity(entity))
				throw std::runtime_error("Invalid entity");

			uint32_t componentIndex = _sparseIndices[entity.id];
			if (componentIndex >= _denseComponents.size())
				throw std::runtime_error("Invalid component index (index >= size)");

			if(componentIndex == INVALID_COMPONENT_INDEX)
				throw std::runtime_error("Entity does not have this component");

			return _denseComponents[componentIndex];
		}

		void RemoveIfPresent(Entity entity) override
		{
			if (!_validEntity(entity))
				throw std::runtime_error("Invalid entity");

			if(!Contains(entity))
				throw std::runtime_error("Entity does not have this component");

			uint32_t componentIndex = _sparseIndices[entity.id];
			//swap
			std::swap(_denseComponents[componentIndex], _denseComponents.back());
			std::swap(_denseEntities[componentIndex], _denseEntities.back());
			_denseComponents.pop_back();
			_denseEntities.pop_back();

			//update sparse index of the swapped entity
			Entity swappedEntity = _denseEntities[componentIndex];
			_sparseIndices[swappedEntity.id] = componentIndex;
		}

		bool Contains(Entity entity) const override
		{
			if (!_validEntity(entity))
				return false;

			uint32_t componentIndex = _sparseIndices[entity.id];
			return componentIndex != INVALID_COMPONENT_INDEX && componentIndex < _denseComponents.size();
		}
	};
}