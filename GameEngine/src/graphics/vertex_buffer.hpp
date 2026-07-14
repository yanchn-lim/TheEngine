#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

#include "debug/memory_tracker.hpp"

namespace Graphics
{
    class VertexBuffer
    {
	private:
	    uint32_t _id{ 0 };
		Memory::ResourceUsage _memoryUsage;

	public:
        bool Create(const void* data, size_t size, std::string_view label = "Vertex Buffer");
        void Destroy();
        uint32_t GetId() const;
        bool IsValid() const;

        VertexBuffer() = default;
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;
        VertexBuffer(VertexBuffer&&) noexcept;
        VertexBuffer& operator=(VertexBuffer&&) noexcept;
    };
}
