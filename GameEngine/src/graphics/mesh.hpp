#pragma once

#include <cstdint>

namespace Graphics
{
	class Mesh
	{
	private:
		uint32_t _vao{};
		uint32_t _vbo{};
		uint32_t _vertCnt{};
	
	public:
		bool Create(const float* vertices, uint32_t vertexCount, uint32_t floatsPerVertex);
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