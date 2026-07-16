#include <GLFW/glfw3.h>

#include "vulkan_renderer.hpp"
#include "debug/debug.hpp"

namespace Graphics
{
	bool VulkanRenderer::Init(GLFWwindow* window)
	{
		Shutdown();
		_window = window;
		_commands.reserve(8192);

		if (!_window || !_context.Init(_window) || !_device.Init(_context))
		{
			Shutdown();
			return false;
		}

		try
		{
			int width = 0;
			int height = 0;
			glfwGetFramebufferSize(_window, &width, &height);
			if (width <= 0 || height <= 0)
				throw std::runtime_error("Initial Vulkan framebuffer extent is zero");

			_requestedExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			if (!_swapchain.Create(_device, _context.SurfaceHandle(), _requestedExtent))
				throw std::runtime_error("Vulkan swapchain creation failed");

			CreateFrameResources();
			return true;
		}
		catch (const std::exception& error)
		{
			Debug::LogError("VulkanRenderer::Init : ", error.what());
			Shutdown();
			return false;
		}
	}

	void VulkanRenderer::Submit(const DrawCmd& command)
	{
		if (_frameReady)
			_commands.push_back(command);
	}

	void VulkanRenderer::BeginFrame()
	{
		_commands.clear();
		_frameReady = false;

		//if (_fatalError || !TryRecreateSwapchain())
		//	return;
	}

	void VulkanRenderer::EndFrame()
	{
	}

	void VulkanRenderer::Present()
	{
	}

	void VulkanRenderer::SetCamera(const Camera2D& camera)
	{
		_camera = camera;
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		_requestedExtent = { width, height };
		_resizePending = true;
	}

	void VulkanRenderer::Shutdown()
	{
		_context.Shutdown();
	}

	void VulkanRenderer::CreateFrameResources()
	{
		for (VulkanFrameResources& frame : _frames)
		{
			frame.commandPool = vk::raii::CommandPool(
				_device.Device(),
				vk::CommandPoolCreateInfo{
					.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
					.queueFamilyIndex = _device.GraphicsQueueFamily()
				});

			auto commandBuffers = _device.Device().allocateCommandBuffers(
				vk::CommandBufferAllocateInfo{
					.commandPool = *frame.commandPool,
					.level = vk::CommandBufferLevel::ePrimary,
					.commandBufferCount = 1
				});
			frame.commandBuffer = std::move(commandBuffers.front());

			frame.inFlightFence = vk::raii::Fence(
				_device.Device(),
				vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
			frame.imageAvailable = vk::raii::Semaphore(
				_device.Device(), vk::SemaphoreCreateInfo{});
			frame.renderFinished = vk::raii::Semaphore(
				_device.Device(), vk::SemaphoreCreateInfo{});
		}
	}
	
}
