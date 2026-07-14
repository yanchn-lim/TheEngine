#include "vulkan_renderer.hpp"

namespace Graphics
{
	bool VulkanRenderer::Init(GLFWwindow* window)
	{
		return _context.Init(window);
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

	void VulkanRenderer::Present()
	{
	}

	void VulkanRenderer::SetCamera(const Camera2D&)
	{
	}

	void VulkanRenderer::OnResize(uint32_t, uint32_t)
	{
	}

	void VulkanRenderer::Shutdown()
	{
		_context.Shutdown();
	}
	
}
