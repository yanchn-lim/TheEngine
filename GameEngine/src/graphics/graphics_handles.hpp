#pragma once

#include <cstdint>

namespace Ludus::Graphics
{
    // identifies one resource slot and rejects stale or foreign device handles
    template<typename Tag>
    struct Handle
    {
        uint32_t index = 0;
        uint32_t generation = 0;
        uint32_t owner = 0;

        bool IsValid() const { return index != 0 && generation != 0 && owner != 0; }
        explicit operator bool() const { return IsValid(); }
        bool operator==(const Handle& other) const
        {
            return index == other.index && generation == other.generation && owner == other.owner;
        }

        bool operator!=(const Handle& other) const { return !(*this == other); }
    };

    // separate types prevent one GPU resource kind from being used as another
    using GpuBufferHandle = Handle<struct GpuBufferTag>;
    using GpuTextureHandle = Handle<struct GpuTextureTag>;
    using GpuSamplerHandle = Handle<struct GpuSamplerTag>;
    using GpuShaderHandle = Handle<struct GpuShaderTag>;
    using GpuPipelineHandle = Handle<struct GpuPipelineTag>;
}
