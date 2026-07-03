#pragma once

#include <cstdint>
#include "vertex_array.hpp"

namespace Graphics
{
	class Mesh
	{
	private:
		//uint32_t _vao{};
		//uint32_t _vbo{};
		uint32_t _vertCnt{};
	
		VertexArray _vertexArray;
		VertexBuffer _vertexBuffer;
		VertexLayout _vertexLayout;

	public:
		bool Create(const float* vertices, uint32_t vertexCount, uint32_t floatsPerVertex);
		//bool Create()
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