#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Graphics
{
	class Mesh;
	struct Material;

	struct DrawCmd
	{
		const Mesh* mesh{ nullptr };
		const Material* material = nullptr;
		glm::mat4 transform{ 1.0f };
	};
}
