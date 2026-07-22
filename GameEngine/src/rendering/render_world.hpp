#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "render_item.hpp"

namespace Rendering
{
    // identifies persistent render data without exposing storage addresses
    struct RenderInstanceHandle
    {
        uint32_t index = 0;
        uint32_t generation = 0;
        bool IsValid() const { return index != 0 && generation != 0; }
        explicit operator bool() const { return IsValid(); }
    };

    // owns persistent render instances and one-frame render submissions
    class RenderWorld
    {
    public:
        // persistent operations are used by entities and direct code objects
        RenderInstanceHandle CreateMeshInstance(const MeshInstanceDesc& desc);
        RenderInstanceHandle CreateSpriteInstance(const SpriteInstanceDesc& desc);
        bool UpdateMeshInstance(RenderInstanceHandle handle, const MeshInstanceDesc& desc);
        bool UpdateSpriteInstance(RenderInstanceHandle handle, const SpriteInstanceDesc& desc);
        bool SetTransform(RenderInstanceHandle handle, const glm::mat4& transform);
        bool SetVisible(RenderInstanceHandle handle, bool visible);
        bool SetMesh(RenderInstanceHandle handle, Assets::MeshHandle mesh);
        bool SetMaterial(RenderInstanceHandle handle, Assets::MaterialHandle material);
        bool SetLayers(RenderInstanceHandle handle, RenderLayerMask layers);
        bool Destroy(RenderInstanceHandle handle);

        // transient operations copy data that is discarded after frame execution
        void DrawMeshOnce(const MeshInstanceDesc& desc);
        void DrawSpriteOnce(const SpriteInstanceDesc& desc);
        void Collect(RenderLayerMask viewLayers, std::vector<RenderItem>& output) const;
        void EndFrame();
        void Clear();

    private:
        struct Slot
        {
            std::optional<RenderItem> item;
            uint32_t generation = 1;
        };

        // slots retain generations while transient items use frame-owned storage
        std::vector<Slot> _slots;
        std::vector<RenderItem> _transient;

        RenderInstanceHandle Create(RenderItem item);
        Slot* Get(RenderInstanceHandle handle);
        const Slot* Get(RenderInstanceHandle handle) const;
    };
}
