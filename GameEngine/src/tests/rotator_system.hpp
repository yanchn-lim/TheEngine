#pragma once

#include "ecs/ecs_system.hpp"

namespace Tests
{
	class RotatorSystem final : public ECS::ISystem
	{
	public:
		static constexpr ECS::SystemPhase Phase = ECS::SystemPhase::UPDATE;
		static constexpr int Order = 100;

		void OnFixedUpdate(ECS::World& world, double fixedDeltaTime) override;
	};
}
