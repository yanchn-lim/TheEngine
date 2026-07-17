#pragma once

#include "graphics/irenderer.hpp"
#include "graphics/vulkan/vulkan_context.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_frameresources.hpp"

namespace Graphics
{
	class VulkanRenderer final : public IRenderer
	{
	public:
		bool Init(GLFWwindow*) override;
		void Submit(const DrawCmd&) override;
		void BeginFrame() override;
		void EndFrame() override;
		void Present() override;
		void SetCamera(const Camera2D& camera) override;
		void OnResize(uint32_t width, uint32_t height) override;
		void Shutdown() override;

	private:
		GLFWwindow* _window = nullptr;
		VulkanContext _context;
		VulkanDevice _device;
		VulkanSwapchain _swapchain;
		std::array<VulkanFrameResources, FramesInFlight> _frames;
		std::vector<DrawCmd> _commands;
		Camera2D _camera;
		uint32_t _frameIndex = 0;
		uint32_t _imageIndex = 0;
		bool _frameReady = false;
		bool _renderingActive = false;
		bool _resizePending = false;
		bool _fatalError = false;
		vk::Extent2D _requestedExtent{};

		void CreateFrameResources();
		bool TryRecreateSwapchain();
		void BeginSwapchainRendering(vk::raii::CommandBuffer& commandBuffer);
		void EndSwapchainRendering(vk::raii::CommandBuffer& commandBuffer);
	};
}