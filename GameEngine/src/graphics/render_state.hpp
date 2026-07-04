#pragma once

#include "blend_mode.hpp"

namespace Graphics
{
	struct RenderState
	{
		bool depthTest = false;
		bool depthWrite = false;
		BlendMode blendMode = BlendMode::ALPHA;
		bool culling = false;
	};
}
