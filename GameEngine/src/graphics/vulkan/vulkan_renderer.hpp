#pragma once

#include "graphics/irenderer.hpp"
#include "graphics/vulkan/vulkan_context.hpp"

namespace Graphics
{
	class VulkanRenderer : public IRenderer
	{
	private:
		VulkanContext _context;

	public:
		bool Init(GLFWwindow*) override;
		void Submit(const DrawCmd&) override;
		void BeginFrame() override;
		void EndFrame() override;
		void Present() override;
		void SetCamera(const Camera2D& camera) override;
		void OnResize(uint32_t width, uint32_t height) override;
		void Shutdown() override;
	};
}