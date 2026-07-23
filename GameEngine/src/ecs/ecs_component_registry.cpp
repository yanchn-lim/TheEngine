#include "ecs_component_registry.hpp"

namespace ECS
{
	template<typename Component>
	Component& ComponentRegistry<Component>::AddComponent(Entity entity, const Component& component)
	{
		//check entity validity
		if(!entity.IsValid())
			throw std::runtime_error("Invalid entity");

		if (!_freeIndices.empty())
		{
			//use a free index
			uint32_t componentIndex = _freeIndices.back();
			_freeIndices.pop_back();
			_entityToComponentIndex[entity.id] = componentIndex;
			_components[componentIndex] = component;
		}
		else
		{
			//add new component
			uint32_t componentIndex = static_cast<uint32_t>(_components.size());
			_components.push_back(component);
			if (entity.id >= _entityToComponentIndex.size())
				_entityToComponentIndex.resize(entity.id + 1, UINT32_MAX); //resize and initialize with invalid index
			_entityToComponentIndex[entity.id] = componentIndex;
		}
	}

	template<typename Component>
	void ComponentRegistry<Component>::RemoveComponent(Entity entity)
	{
		if (!entity.IsValid() || entity.id >= _entityToComponentIndex.size())
			throw std::runtime_error("Invalid entity");

		uint32_t componentIndex = _entityToComponentIndex[entity.id];
		_freeIndices.push_back(componentIndex);
		_entityToComponentIndex[entity.id] = INVALID_COMPONENT_INDEX;
	}

	template<typename Component>
	Component& ComponentRegistry<Component>::GetComponent(Entity entity)
	{
		if(!entity.IsValid() || entity.id >= _entityToComponentIndex.size())
			throw std::runtime_error("Invalid entity");

		uint32_t componentIndex = _entityToComponentIndex[entity.id];
		if (componentIndex >= _components.size())
			throw std::runtime_error("Invalid component index");

		return _components[componentIndex];
	}


}