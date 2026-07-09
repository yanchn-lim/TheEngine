#pragma once

#include <cstdint>

#include "assets/mesh_import_data.hpp"

namespace Assets::Primitive2D
{
	MeshImportData Triangle();
	MeshImportData Quad();
	MeshImportData Circle(uint32_t segments = 32);
}
