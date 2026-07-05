#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "glm/glm.hpp"

#include "debug/debug.hpp"
#include "graphics/primitive_topology.hpp"
#include "graphics/vertex_layout.hpp"

namespace Assets
{
	struct MeshVertex
	{
		glm::vec3 position{};
		glm::vec3 color{ 1.f };
		glm::vec2 texCoord0{};
	};

	struct MeshSourceData
	{
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		Graphics::PrimitiveTopology topology = Graphics::PrimitiveTopology::TRIANGLES;
	};

	Graphics::VertexLayout CreateMeshVertexLayout();

	struct MeshSourceCollection
	{
		std::vector<MeshSourceData> meshes;
	};

}
