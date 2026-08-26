#pragma once

#include "ecs_entity.hpp"
#include <cstddef>
#include <span>

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
			virtual bool Contains(Entity entity) const = 0;
			virtual std::size_t GetSize() const noexcept = 0;
			virtual std::span<const Entity> GetEntities() const noexcept = 0;
		};
	}
}
