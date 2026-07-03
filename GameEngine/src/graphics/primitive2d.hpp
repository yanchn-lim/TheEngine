#pragma once

#include <cstdint>
#include <vector>
#include "mesh_data.hpp"

namespace Graphics::Primitive2D
{
	constexpr uint32_t FloatsPerVertex = 8;

	Graphics::MeshData Triangle();
	Graphics::MeshData Quad();
	std::vector<float> CreateCircle(uint32_t segments = 32);
}
