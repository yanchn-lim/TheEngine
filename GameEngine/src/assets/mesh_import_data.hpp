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
	// this vertex format is shared by importers, shaders, and gpu upload.
	struct MeshVertex
	{
		glm::vec3 position{};
		glm::vec3 color{ 1.f };
		glm::vec2 texCoord0{};
	};

	// one surface is one draw and can select its own material.
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
