#include "render_world.hpp"

#include <algorithm>

namespace Rendering
{
    namespace
    {
        // apply visibility and view-layer rejection before queue construction
        bool IsVisible(const RenderItem& item, RenderLayerMask layers)
        {
            return std::visit([layers](const auto& desc)
            {
                return desc.visible && (desc.layers & layers) != 0;
            }, item);
        }

        int32_t SortOrder(const RenderItem& item)
        {
            return std::visit([](const auto& desc) { return desc.sortingOrder; }, item);
        }
    }

    RenderInstanceHandle RenderWorld::CreateMeshInstance(const MeshInstanceDesc& desc) { return Create(desc); }
    RenderInstanceHandle RenderWorld::CreateSpriteInstance(const SpriteInstanceDesc& desc) { return Create(desc); }

    bool RenderWorld::UpdateMeshInstance(RenderInstanceHandle handle, const MeshInstanceDesc& desc)
    {
        Slot* slot = Get(handle);
        if (!slot || !std::holds_alternative<MeshInstanceDesc>(*slot->item)) return false;
        slot->item = desc;
        return true;
    }

    bool RenderWorld::UpdateSpriteInstance(RenderInstanceHandle handle, const SpriteInstanceDesc& desc)
    {
        Slot* slot = Get(handle);
        if (!slot || !std::holds_alternative<SpriteInstanceDesc>(*slot->item)) return false;
        slot->item = desc;
        return true;
    }

    bool RenderWorld::SetTransform(RenderInstanceHandle handle, const glm::mat4& transform)
    {
        Slot* slot = Get(handle);
        if (!slot) return false;
        std::visit([&transform](auto& desc) { desc.transform = transform; }, *slot->item);
        return true;
    }

    bool RenderWorld::SetVisible(RenderInstanceHandle handle, bool visible)
    {
        Slot* slot = Get(handle);
        if (!slot) return false;
        std::visit([visible](auto& desc) { desc.visible = visible; }, *slot->item);
        return true;
    }

    bool RenderWorld::SetMesh(RenderInstanceHandle handle, Assets::MeshHandle mesh)
    {
        Slot* slot = Get(handle);
        if (!slot || !std::holds_alternative<MeshInstanceDesc>(*slot->item)) return false;
        std::get<MeshInstanceDesc>(*slot->item).mesh = mesh;
        return true;
    }

    bool RenderWorld::SetMaterial(RenderInstanceHandle handle, Assets::MaterialHandle material)
    {
        Slot* slot = Get(handle);
        if (!slot) return false;
        std::visit([material](auto& desc) { desc.material = material; }, *slot->item);
        return true;
    }

    bool RenderWorld::SetLayers(RenderInstanceHandle handle, RenderLayerMask layers)
    {
        Slot* slot = Get(handle);
        if (!slot) return false;
        std::visit([layers](auto& desc) { desc.layers = layers; }, *slot->item);
        return true;
    }

    bool RenderWorld::Destroy(RenderInstanceHandle handle)
    {
        Slot* slot = Get(handle);
        if (!slot) return false;
        slot->item.reset();
        ++slot->generation;
        if (slot->generation == 0) slot->generation = 1;
        return true;
    }

    void RenderWorld::DrawMeshOnce(const MeshInstanceDesc& desc) { _transient.emplace_back(desc); }
    void RenderWorld::DrawSpriteOnce(const SpriteInstanceDesc& desc) { _transient.emplace_back(desc); }

    void RenderWorld::Collect(RenderLayerMask viewLayers, std::vector<RenderItem>& output) const
    {
        output.clear();
        for (const Slot& slot : _slots)
            if (slot.item && IsVisible(*slot.item, viewLayers)) output.push_back(*slot.item);
        for (const RenderItem& item : _transient)
            if (IsVisible(item, viewLayers)) output.push_back(item);
        std::stable_sort(output.begin(), output.end(), [](const RenderItem& left, const RenderItem& right)
        {
            return SortOrder(left) < SortOrder(right);
        });
    }

    void RenderWorld::EndFrame() { _transient.clear(); }
    void RenderWorld::Clear() { _transient.clear(); _slots.clear(); }

    RenderInstanceHandle RenderWorld::Create(RenderItem item)
    {
        // reuse destroyed slots without making their old handles valid again
        for (uint32_t index = 0; index < _slots.size(); ++index)
        {
            Slot& slot = _slots[index];
            if (slot.item) continue;
            slot.item = std::move(item);
            return { index + 1, slot.generation };
        }
        Slot& slot = _slots.emplace_back();
        slot.item = std::move(item);
        return { static_cast<uint32_t>(_slots.size()), slot.generation };
    }

    RenderWorld::Slot* RenderWorld::Get(RenderInstanceHandle handle)
    {
        // the generation check rejects stale handles after destruction
        if (!handle || handle.index > _slots.size()) return nullptr;
        Slot& slot = _slots[handle.index - 1];
        return slot.generation == handle.generation && slot.item ? &slot : nullptr;
    }

    const RenderWorld::Slot* RenderWorld::Get(RenderInstanceHandle handle) const
    {
        return const_cast<RenderWorld*>(this)->Get(handle);
    }
}
