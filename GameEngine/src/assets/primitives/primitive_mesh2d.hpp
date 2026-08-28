#pragma once

#include <cstdint>

#include "assets/mesh_import_data.hpp"

namespace Ludus::Assets::Primitive2D
{
	// generates standard CPU meshes that follow the same upload path as imported assets
	MeshSurface Triangle();
	MeshSurface FullscreenTriangle();
	MeshSurface Quad();
	MeshSurface Circle(uint32_t segments = 32);
}
