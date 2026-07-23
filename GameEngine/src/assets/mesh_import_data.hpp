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
	// describes the engine vertex format produced by model and primitive importers
	struct MeshVertex
	{
		glm::vec3 position{};
		glm::vec3 color{ 1.f };
		glm::vec2 texCoord0{};
	};

	// keeps imported geometry on the CPU until the render resource manager needs it
	struct MeshImportData
	{
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		Graphics::PrimitiveTopology topology = Graphics::PrimitiveTopology::TRIANGLES;
	};

	Graphics::VertexLayout CreateMeshVertexLayout();

	// one imported model can contain several independently drawable meshes
	struct ModelImportData
	{
		std::vector<MeshImportData> meshes;
	};

}
