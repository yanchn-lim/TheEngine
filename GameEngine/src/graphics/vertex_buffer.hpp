#pragma once

#include <cstdint>
#include <cstddef>

namespace Graphics
{
    class VertexBuffer
    {
    private:
	    uint32_t _id{ 0 };

    public:
        bool Create(const void* data, size_t size);
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