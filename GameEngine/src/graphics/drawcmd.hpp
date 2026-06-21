#pragma once

#include <cstdint>

namespace Graphics
{
	class Shader;

	struct DrawCmd
	{
		uint32_t vao = 0;
		const Shader* shader{nullptr};

		uint32_t vertcnt = 0;
	};
}
