#include "movement_system.hpp"

namespace Tests
{


    void MovementSystem::OnUpdate(ECS::World& world)
    {
        world.ForEach<Position, Velocity>(
            [](ECS::Entity,
                Position& position,
                Velocity& velocity)
            {
                position.x += velocity.x;
                position.y += velocity.y;
            });
    }
}