#pragma once

#include "ecs_entity.hpp"

namespace ECS
{
	//every type of component pool should inherit from this interface
	class IComponentPool
	{
	public:
		//functions of generic component pool
		virtual ~IComponentPool() = default;
		virtual void RemoveIfPresent(Entity entity) = 0;
		virtual bool Contains(Entity entity) const = 0;
	};
}