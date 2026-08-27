#pragma once

#include <span>
#include "ecs_entity.hpp"

namespace Ludus::ECS
{
	template<typename Component>
	struct ComponentView
	{
		std::span<const Entity> entities;
		std::span<Component> components;
	};
}