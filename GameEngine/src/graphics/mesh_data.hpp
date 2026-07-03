#pragma once

#include "vertex_layout.hpp"
#include "primitive_topology.hpp"

namespace Graphics
{
	struct MeshData
	{
		const void* vertices = nullptr;
		uint32_t vertexCount = 0;

		const uint32_t* indices = nullptr;
		uint32_t indexCount = 0;

		VertexLayout layout;

		PrimitiveTopology topology = PrimitiveTopology::TRIANGLES;
	};
}