#pragma once

#include "ecs_entity.hpp"

namespace ECS
{
	constexpr inline uint32_t INVALID_COMPONENT_INDEX = 0;

	template<typename Component>
	class ComponentRegistry
	{
	private:
		//collection of components
		std::vector<Component> _components{0};
		std::vector<uint32_t> _freeIndices;
		std::vector<uint32_t> _entityToComponentIndex{0}; //same size as entity slots, maps entity id to component index
	public:
		ComponentRegistry() = default;
		~ComponentRegistry() = default;

		Component& AddComponent(Entity entity, const Component& component);
		void RemoveComponent(Entity entity);
		Component& GetComponent(Entity entity);
	};
}