#pragma once

#include "ecs_entity.hpp"

namespace ECS
{
	class World;

	namespace Internal
	{
		class IComponentPool
		{
			friend class ECS::World;

		public:
			virtual ~IComponentPool() = default;

		private:
			virtual bool RemoveIfPresent(Entity entity) = 0;
		};
	}
}
