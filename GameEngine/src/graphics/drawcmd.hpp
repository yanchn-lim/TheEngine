#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Graphics
{
	class Shader;
	class Mesh;

	struct DrawCmd
	{
		const Mesh* mesh{ nullptr };
		const Shader* shader{nullptr};

		glm::mat4 transform{ 1.0f };
	};
}
