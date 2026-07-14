#include "vulkan_renderer.hpp"

namespace Graphics
{
	bool VulkanRenderer::Init(GLFWwindow* window)
	{
		if (!_context.Init(window))
		{
			return false;
		}

		return true;
	}

	void VulkanRenderer::Submit(const DrawCmd&)
	{
	}

	void VulkanRenderer::BeginFrame()
	{
	}

	void VulkanRenderer::EndFrame()
	{
	}

	void VulkanRenderer::SetCamera(const Camera2D& camera)
	{
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
	}

	void VulkanRenderer::Shutdown()
	{
	}
}