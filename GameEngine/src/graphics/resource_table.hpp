#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace Graphics
{
    template<typename HandleType>
    uint32_t AcquireResourceTableOwner()
    {
        static std::atomic<uint32_t> nextOwner{ 0 };
        return ++nextOwner;
    }

    // owns backend resources and turns slot reuse into generation-safe handles
    template<typename HandleType, typename ResourceType>
    class ResourceTable
    {
    public:
        ResourceTable() : _owner(AcquireResourceTableOwner<HandleType>()) {}
        ResourceTable(const ResourceTable&) = delete;
        ResourceTable& operator=(const ResourceTable&) = delete;

        template<typename... Args>
        HandleType Create(Args&&... args)
        {
            // reuse an empty slot while advancing generations on destruction
            for (uint32_t slotIndex = 0; slotIndex < _slots.size(); ++slotIndex)
            {
                Slot& slot = _slots[slotIndex];
                if (slot.resource)
                    continue;

                slot.resource.emplace(std::forward<Args>(args)...);
                return { slotIndex + 1, slot.generation, _owner };
            }

            Slot& slot = _slots.emplace_back();
            slot.resource.emplace(std::forward<Args>(args)...);
            return { static_cast<uint32_t>(_slots.size()), slot.generation, _owner };
        }

        ResourceType* Get(HandleType handle)
        {
            // owner checks reject handles created by another device table
            if (!handle || handle.owner != _owner || handle.index > _slots.size())
                return nullptr;

            Slot& slot = _slots[handle.index - 1];
            if (slot.generation != handle.generation || !slot.resource)
                return nullptr;

            return &*slot.resource;
        }

        const ResourceType* Get(HandleType handle) const
        {
            return const_cast<ResourceTable*>(this)->Get(handle);
        }

        bool Destroy(HandleType handle)
        {
            // resetting the optional releases the native owner before slot reuse
            if (!handle || handle.owner != _owner || handle.index > _slots.size())
                return false;

            Slot& slot = _slots[handle.index - 1];
            if (slot.generation != handle.generation || !slot.resource)
                return false;

            slot.resource.reset();
            ++slot.generation;
            if (slot.generation == 0)
                slot.generation = 1;
            return true;
        }

        void Clear()
        {
            _slots.clear();
            _owner = AcquireResourceTableOwner<HandleType>();
        }

        template<typename Function>
        void ForEach(Function&& function)
        {
            for (Slot& slot : _slots)
                if (slot.resource) function(*slot.resource);
        }

    private:
        struct Slot
        {
            std::optional<ResourceType> resource;
            uint32_t generation = 1;
        };

        std::vector<Slot> _slots;
        uint32_t _owner = 0;
    };
}
