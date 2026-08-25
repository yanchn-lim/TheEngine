#pragma once


#include "ecs/ecs_world.hpp"

namespace Ludus
{
	class Scene
	{
	public:
		ECS::World& GetWorld() noexcept;
		const ECS::World& GetWorld() const noexcept;

		void FixedUpdate(double fixedDeltaTime);
		void Update();
	
	private:
		ECS::World _world{};
	};
}
