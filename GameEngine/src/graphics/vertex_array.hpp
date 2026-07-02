#pragma once

#include <cstdint>

class VertexBuffer;
class VertexLayout;
class IndexBuffer;

namespace Graphics
{
	class VertexArray
	{
	private:
		uint32_t _id{ 0 };

	public:
        bool Create();
        void Bind() const;
        void Destroy();
        bool IsValid() const;
        uint32_t GetId() const;

        //void SetVertexBuffer(const VertexBuffer& buffer, const VertexLayout& layout);
        void SetIndexBuffer(const IndexBuffer& buffer);

        VertexArray() = default;
        ~VertexArray();

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&&) noexcept;
        VertexArray& operator=(VertexArray&&) noexcept;
	};
}