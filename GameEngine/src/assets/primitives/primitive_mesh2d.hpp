#pragma once

#include <cstdint>

#include "assets/mesh_source_data.hpp"

namespace Assets::Primitive2D
{
	MeshSourceData Triangle();
	MeshSourceData Quad();
	MeshSourceData Circle(uint32_t segments = 32);
}
