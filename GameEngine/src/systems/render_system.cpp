#include "render_system.hpp"

namespace Systems
{
    bool RenderSystem::Add(uint64_t entityId, const Transform2D& transform,
        MeshRendererComponent& component, uint64_t changeVersion)
    {
        // an existing handle turns a repeated add event into an update
        if (component.renderInstance)
            return Update(entityId, transform, component, changeVersion);
        Rendering::MeshInstanceDesc desc;
        desc.mesh = component.mesh;
        desc.material = component.material;
        desc.transform = transform.GetMatrix();
        desc.layers = component.layers;
        desc.entityId = entityId;
        desc.visible = component.visible;
        desc.castShadows = component.castShadows;
        component.renderInstance = _world.CreateMeshInstance(desc);
        component.renderVersion = changeVersion;
        return static_cast<bool>(component.renderInstance);
    }

    bool RenderSystem::Add(uint64_t entityId, const Transform2D& transform,
        SpriteComponent& component, Assets::MaterialHandle fallbackMaterial, uint64_t changeVersion)
    {
        if (component.renderInstance)
            return Update(entityId, transform, component, fallbackMaterial, changeVersion);
        Rendering::SpriteInstanceDesc desc;
        desc.texture = component.texture;
        desc.material = component.material ? component.material : fallbackMaterial;
        desc.transform = transform.GetMatrix();
        desc.tint = component.color;
        desc.uvRect = { component.uvMin, component.uvMax };
        desc.sortingOrder = component.sortingOrder;
        desc.entityId = entityId;
        desc.visible = component.visible;
        component.renderInstance = _world.CreateSpriteInstance(desc);
        component.renderVersion = changeVersion;
        return static_cast<bool>(component.renderInstance);
    }

    bool RenderSystem::Update(uint64_t entityId, const Transform2D& transform,
        MeshRendererComponent& component, uint64_t changeVersion)
    {
        // unchanged versions avoid rebuilding identical render descriptions
        if (!component.renderInstance) return Add(entityId, transform, component, changeVersion);
        if (component.renderVersion == changeVersion) return true;
        Rendering::MeshInstanceDesc desc;
        desc.mesh = component.mesh;
        desc.material = component.material;
        desc.transform = transform.GetMatrix();
        desc.layers = component.layers;
        desc.entityId = entityId;
        desc.visible = component.visible;
        desc.castShadows = component.castShadows;
        if (!_world.UpdateMeshInstance(component.renderInstance, desc)) return false;
        component.renderVersion = changeVersion;
        return true;
    }

    bool RenderSystem::Update(uint64_t entityId, const Transform2D& transform,
        SpriteComponent& component, Assets::MaterialHandle fallbackMaterial, uint64_t changeVersion)
    {
        if (!component.renderInstance)
            return Add(entityId, transform, component, fallbackMaterial, changeVersion);
        if (component.renderVersion == changeVersion) return true;
        Rendering::SpriteInstanceDesc desc;
        desc.texture = component.texture;
        desc.material = component.material ? component.material : fallbackMaterial;
        desc.transform = transform.GetMatrix();
        desc.tint = component.color;
        desc.uvRect = { component.uvMin, component.uvMax };
        desc.sortingOrder = component.sortingOrder;
        desc.entityId = entityId;
        desc.visible = component.visible;
        if (!_world.UpdateSpriteInstance(component.renderInstance, desc)) return false;
        component.renderVersion = changeVersion;
        return true;
    }

    void RenderSystem::Remove(MeshRendererComponent& component)
    {
        // clear runtime bookkeeping after the RenderWorld instance is destroyed
        if (component.renderInstance) _world.Destroy(component.renderInstance);
        component.renderInstance = {};
        component.renderVersion = 0;
    }

    void RenderSystem::Remove(SpriteComponent& component)
    {
        if (component.renderInstance) _world.Destroy(component.renderInstance);
        component.renderInstance = {};
        component.renderVersion = 0;
    }
}
