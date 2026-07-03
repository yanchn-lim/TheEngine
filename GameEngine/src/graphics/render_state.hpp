#pragma once

namespace Graphics
{
	struct RenderState
	{
		bool depthTest = false;
		bool depthWrite = false;
		bool blending = true;
		bool culling = false;
	};
}
