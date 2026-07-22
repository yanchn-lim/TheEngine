#pragma once

#include <cstdint>

#include "components/mesh_renderer_component.hpp"
#include "components/sprite_component.hpp"
#include "components/transform2d.hpp"
#include "rendering/render_world.hpp"

namespace Systems
{
    // converts ECS component lifecycle events into persistent RenderWorld instances
    class RenderSystem
    {
    public:
        explicit RenderSystem(Rendering::RenderWorld& world) : _world(world) {}

        // Add creates the runtime instance stored beside serialized component data
        bool Add(uint64_t entityId, const Transform2D& transform, MeshRendererComponent& component,
            uint64_t changeVersion);
        bool Add(uint64_t entityId, const Transform2D& transform, SpriteComponent& component,
            Assets::MaterialHandle fallbackMaterial, uint64_t changeVersion);
        // Update applies changed component and transform data to the existing instance
        bool Update(uint64_t entityId, const Transform2D& transform, MeshRendererComponent& component,
            uint64_t changeVersion);
        bool Update(uint64_t entityId, const Transform2D& transform, SpriteComponent& component,
            Assets::MaterialHandle fallbackMaterial, uint64_t changeVersion);
        // Remove destroys the runtime instance when its component or entity leaves the ECS
        void Remove(MeshRendererComponent& component);
        void Remove(SpriteComponent& component);

    private:
        Rendering::RenderWorld& _world;
    };
}
