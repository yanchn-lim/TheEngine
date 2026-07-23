#pragma once

#include <cstdint>

#include "assets/mesh_import_data.hpp"

namespace Assets::Primitive2D
{
	// generates standard CPU meshes that follow the same upload path as imported assets
	MeshImportData Triangle();
	MeshImportData Quad();
	MeshImportData Circle(uint32_t segments = 32);
}
