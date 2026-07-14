#pragma once

#include <cstdint>
#include <string_view>
#include "index_buffer.hpp"
#include "vertex_array.hpp"
#include "mesh_upload_data.hpp"
#include "primitive_topology.hpp"

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

		PrimitiveTopology _topology = PrimitiveTopology::TRIANGLES; //default to triangle

	public:
		bool Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout, std::string_view label = "Mesh");
		bool Create(const void* vertexData, uint32_t vertexCount, const VertexLayout& layout, const uint32_t* indices, uint32_t indexCount, std::string_view label = "Mesh");
		bool Create(const MeshUploadData& data, std::string_view label = "Mesh");

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
