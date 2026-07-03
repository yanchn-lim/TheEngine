#pragma once

#include "render_state.hpp"

namespace Graphics
{
	class Shader;
	class Texture2D;

	struct Material
	{
		const Shader* shader = nullptr;
		const Texture2D* texture = nullptr;
		RenderState state{};
	};
}