#pragma once

#include "ecs/ecs_system.hpp"
#include "ecs/ecs_world.hpp"
#include "velocity.hpp"
#include "position.hpp"

namespace Tests
{
	class MovementSystem final : public ECS::ISystem
	{
    public:
        static constexpr ECS::SystemPhase Phase =
            ECS::SystemPhase::UPDATE;

        static constexpr int Order = 100;

        void OnUpdate(ECS::World& world) override;
	};
}