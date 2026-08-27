#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "asset_handle.hpp"
#include "graphics/primitive_topology.hpp"
#include "graphics/vertex_layout.hpp"

namespace Ludus::Assets
{
	// describes the engine vertex format produced by model and primitive importers
	struct MeshVertex
	{
		glm::vec3 position{};
		glm::vec3 color{ 1.f };
		glm::vec2 texCoord0{};
	};

	// one independently drawable section of a mesh
	struct MeshSurface
	{
		std::string name;
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		Ludus::Graphics::PrimitiveTopology topology = Ludus::Graphics::PrimitiveTopology::TRIANGLES;
		MaterialHandle material;
	};

	// temporary result produced by a mesh importer
	struct MeshImportData
	{
		std::vector<MeshSurface> surfaces;
	};

	Ludus::Graphics::VertexLayout CreateMeshVertexLayout();
}
