#pragma once

#include <cstdint>
#include <vector>

namespace Graphics::Primitive2D
{
	constexpr uint32_t FloatsPerVertex = 8;

	struct MeshData
	{
		const float* vertices{ nullptr };
		uint32_t vertexCount{ 0 };
		uint32_t floatsPerVertex{ FloatsPerVertex };
	};

	MeshData Triangle();
	MeshData Quad();
	std::vector<float> CreateCircle(uint32_t segments = 32);
}
