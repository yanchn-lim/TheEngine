#pragma once

#include <span>
#include "ecs_entity.hpp"

namespace Ludus::ECS
{
	// these spans borrow one component pool's dense storage.
	// a structural change to that pool can invalidate both spans.
	template<typename Component>
	struct ComponentView
	{
		std::span<const Entity> entities;
		std::span<Component> components;
	};
}