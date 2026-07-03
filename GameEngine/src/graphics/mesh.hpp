#pragma once

#include <cstdint>
#include "index_buffer.hpp"
#include "vertex_array.hpp"

namespace Graphics
{
	class Mesh
	{
	private:
		uint32_t _vertCnt{};
	
		VertexArray _vertexArray;
		VertexBuffer _vertexBuffer;
		VertexLayout _vertexLayout;
		IndexBuffer _indexBuffer;

	public:
		bool Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout);
		bool Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout, const uint32_t* indices, uint32_t indexCount);

		void Bind() const;
		void Draw() const;
		void Destroy();

		uint32_t GetVertexCount() const;
		bool IsValid() const;

		Mesh() = default;
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&&) noexcept;
		Mesh& operator=(Mesh&&) noexcept;
	};
}