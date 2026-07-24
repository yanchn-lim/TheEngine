#pragma once

#include "ecs_component_pool_interface.hpp"

namespace ECS
{
	template<typename Component>
	class ComponentPool : public IComponentPool
	{
		std::vector<Component> _components{ 0 };
		std::vector<uint32_t> _freeIndices;
		std::vector<uint32_t> _entityToComponentIndex{ 0 };
	public:
		ComponentPool() = default;

		Component& AddComponent(Entity entity, const Component& component)
		{
			//check entity validity
			if (!entity.IsValid())
				throw std::runtime_error("Invalid entity");

			if (!_freeIndices.empty())
			{
				//use a free index
				uint32_t componentIndex = _freeIndices.back();
				_freeIndices.pop_back();

				if (entity.id >= _entityToComponentIndex.size())
					_entityToComponentIndex.resize(entity.id + 1, INVALID_COMPONENT_INDEX); //resize and initialize with invalid index

				_entityToComponentIndex[entity.id] = componentIndex;
				_components[componentIndex] = component;
				return _components[componentIndex];
			}
			else
			{
				//add new component
				uint32_t componentIndex = static_cast<uint32_t>(_components.size());
				_components.push_back(component);
				if (entity.id >= _entityToComponentIndex.size())
					_entityToComponentIndex.resize(entity.id + 1, INVALID_COMPONENT_INDEX); //resize and initialize with invalid index
				_entityToComponentIndex[entity.id] = componentIndex;

				return _components[componentIndex];
			}
		}

		void RemoveComponent(Entity entity)
		{
			if (!entity.IsValid() || entity.id >= _entityToComponentIndex.size())
				throw std::runtime_error("Invalid entity");

			uint32_t componentIndex = _entityToComponentIndex[entity.id];

			if (componentIndex == INVALID_COMPONENT_INDEX)
				throw std::runtime_error("Entity does not have this component");

			_freeIndices.push_back(componentIndex);
			_entityToComponentIndex[entity.id] = INVALID_COMPONENT_INDEX;
		}

		Component& GetComponent(Entity entity)
		{
			if (!entity.IsValid() || entity.id >= _entityToComponentIndex.size())
				throw std::runtime_error("Invalid entity");

			uint32_t componentIndex = _entityToComponentIndex[entity.id];
			if (componentIndex >= _components.size() || componentIndex == INVALID_COMPONENT_INDEX)
				throw std::runtime_error("Invalid component index");

			return _components[componentIndex];
		}
	};
}