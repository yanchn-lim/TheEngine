#pragma once

#include "blend_mode.hpp"

namespace Ludus::Graphics
{
	// collects fixed pipeline state shared by materials and both back ends
	struct RenderState
	{
		bool depthTest = false;
		bool depthWrite = false;
		BlendMode blendMode = BlendMode::ALPHA;
		bool culling = false;
	};
}
