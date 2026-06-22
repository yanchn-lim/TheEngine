#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Graphics
{
	class Shader;
	class Mesh;
	class Texture2D;

	struct DrawCmd
	{
		const Mesh* mesh{ nullptr };
		const Shader* shader{nullptr};
		const Texture2D* texture{ nullptr };
		glm::mat4 transform{ 1.0f };
	};
}
