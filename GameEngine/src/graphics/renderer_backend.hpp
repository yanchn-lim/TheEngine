#pragma once

namespace Ludus::Graphics
{
	// selects the concrete graphics and ImGui implementations at startup
	enum class RendererBackend
	{
		OPENGL,
		VULKAN
	};
}
