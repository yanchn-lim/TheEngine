#pragma once

#include <cstdint>

#include "assets/model_data.hpp"

namespace Assets::Primitive2D
{
	ModelMeshData Triangle();
	ModelMeshData Quad();
	ModelMeshData Circle(uint32_t segments = 32);
}
