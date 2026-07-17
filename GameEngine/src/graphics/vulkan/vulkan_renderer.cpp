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

		if (_fatalError || !TryRecreateSwapchain())
			return;

		try
		{
			VulkanFrameResources& frame = _frames[_frameIndex];
			const vk::Result waitResult = _device.Device().waitForFences(
				*frame.inFlightFence, vk::True, UINT64_MAX);
			if (waitResult != vk::Result::eSuccess)
				throw std::runtime_error("Vulkan frame fence wait failed");

			const auto [acquireResult, imageIndex] = _swapchain.Swapchain().acquireNextImage(
				UINT64_MAX, *frame.imageAvailable, nullptr);

			if (acquireResult == vk::Result::eErrorOutOfDateKHR)
			{
				_resizePending = true;
				return;
			}
			if (acquireResult != vk::Result::eSuccess &&
				acquireResult != vk::Result::eSuboptimalKHR)
			{
				throw std::runtime_error("Vulkan swapchain acquisition failed");
			}

			_resizePending |= acquireResult == vk::Result::eSuboptimalKHR;
			_imageIndex = imageIndex;

			_device.Device().resetFences(*frame.inFlightFence);
			frame.commandPool.reset({});
			frame.commandBuffer.begin(vk::CommandBufferBeginInfo{});
			BeginSwapchainRendering(frame.commandBuffer);
			_frameReady = true;
		}
		catch (const std::exception& error)
		{
			Debug::LogError("VulkanRenderer::BeginFrame: ", error.what());
			_fatalError = true;
		}
	}

	void VulkanRenderer::EndFrame()
	{
	}

	void VulkanRenderer::Present()
	{
		if (!_frameReady)
			return;

		try
		{
			VulkanFrameResources& frame = _frames[_frameIndex];
			EndSwapchainRendering(frame.commandBuffer);
			frame.commandBuffer.end();

			vk::CommandBufferSubmitInfo commandInfo{};
			commandInfo.commandBuffer = *frame.commandBuffer;

			vk::SemaphoreSubmitInfo waitInfo{};
			waitInfo.semaphore = *frame.imageAvailable;
			waitInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
			
			vk::SemaphoreSubmitInfo signalInfo{};
			signalInfo.semaphore = *frame.renderFinished;
			signalInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

			vk::SubmitInfo2 submitInfo{};
			submitInfo.waitSemaphoreInfoCount = 1;
			submitInfo.pWaitSemaphoreInfos = &waitInfo;
			submitInfo.commandBufferInfoCount = 1;
			submitInfo.pCommandBufferInfos = &commandInfo;
			submitInfo.signalSemaphoreInfoCount = 1;
			submitInfo.pSignalSemaphoreInfos = &signalInfo;
			_device.GraphicsQueue().submit2(submitInfo, *frame.inFlightFence);

			const vk::SwapchainKHR swapchainHandle = *_swapchain.Swapchain();
			const vk::Semaphore finished = *frame.renderFinished;
			vk::PresentInfoKHR presentInfo{};
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &finished;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchainHandle;
			presentInfo.pImageIndices = &_imageIndex;
			const vk::Result presentResult = _device.PresentQueue().presentKHR(presentInfo);

			_resizePending |= presentResult == vk::Result::eErrorOutOfDateKHR ||
				presentResult == vk::Result::eSuboptimalKHR;
			if (presentResult != vk::Result::eSuccess &&
				presentResult != vk::Result::eSuboptimalKHR &&
				presentResult != vk::Result::eErrorOutOfDateKHR)
			{
				throw std::runtime_error("Vulkan presentation failed");
			}

			_frameReady = false;
			_frameIndex = (_frameIndex + 1) % FramesInFlight;
		}
		catch (const std::exception& error)
		{
			Debug::LogError("VulkanRenderer::Present: ", error.what());
			_fatalError = true;
			_frameReady = false;
		}
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
			vk::CommandPoolCreateInfo cmdPoolCreateInfo{};
			cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			cmdPoolCreateInfo.queueFamilyIndex = _device.GraphicsQueueFamily();
			frame.commandPool = vk::raii::CommandPool(_device.Device(), cmdPoolCreateInfo);

			vk::CommandBufferAllocateInfo cmdBufferAllocateInfo{};
			cmdBufferAllocateInfo.commandPool = *frame.commandPool;
			cmdBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
			cmdBufferAllocateInfo.commandBufferCount = 1;
			
			auto commandBuffers = _device.Device().allocateCommandBuffers(cmdBufferAllocateInfo);

			frame.commandBuffer = std::move(commandBuffers.front());

			vk::FenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
			frame.inFlightFence = vk::raii::Fence(_device.Device(), fenceCreateInfo);
			frame.imageAvailable = vk::raii::Semaphore(_device.Device(), vk::SemaphoreCreateInfo{});
			frame.renderFinished = vk::raii::Semaphore(_device.Device(), vk::SemaphoreCreateInfo{});
		}
	}

	bool VulkanRenderer::TryRecreateSwapchain()
	{
		/*
		TryRecreateSwapchain() checks _resizePending, calls glfwGetFramebufferSize, 
		returns false while either dimension is zero, waits idle, 
		recreates the swapchain plus every extent/format-dependent resource, and clears _resizePending only after success.
		*/

		if (_resizePending)
		{
			int width = 0;
			int height = 0;
			glfwGetFramebufferSize(_window, &width, &height);
			if (width <= 0 || height <= 0)
				return false;

			_requestedExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			_device.WaitIdle();

			if (!_swapchain.Recreate(_device, _context.SurfaceHandle(), _requestedExtent))
			{
				return false;
			}

			_resizePending = false;
		}

		return true;
	}

	void VulkanRenderer::BeginSwapchainRendering(vk::raii::CommandBuffer& commandBuffer)
	{
		vk::ImageMemoryBarrier2 toColor{};
		toColor.srcStageMask = vk::PipelineStageFlagBits2::eNone;
		toColor.srcAccessMask = vk::AccessFlagBits2::eNone;
		toColor.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		toColor.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
		toColor.oldLayout = vk::ImageLayout::eUndefined;
		toColor.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toColor.image = _swapchain.Image(_imageIndex);
		vk::ImageSubresourceRange imgSubresourceRange{};
		imgSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgSubresourceRange.baseMipLevel = 0;
		imgSubresourceRange.levelCount = 1;
		imgSubresourceRange.baseArrayLayer = 0;
		imgSubresourceRange.layerCount = 1;
		toColor.subresourceRange = imgSubresourceRange;

		vk::DependencyInfo dependencyInfo{};
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &toColor;
		commandBuffer.pipelineBarrier2(dependencyInfo);

		vk::ClearValue clear{};
		clear.color.float32 = std::array{ 0.1f, 0.1f, 0.1f, 1.0f };

		vk::RenderingAttachmentInfo colorAttachment{};
		colorAttachment.imageView = _swapchain.ImageView(_imageIndex);
		colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.clearValue = clear;
		
		vk::RenderingInfo renderingInfo{};
		vk::Rect2D renderArea{};
		renderArea.offset = { 0, 0 };
		renderArea.extent = _swapchain.Extent();
		renderingInfo.renderArea = renderArea;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;

		commandBuffer.beginRendering(renderingInfo);
		_renderingActive = true;
	}

	void VulkanRenderer::EndSwapchainRendering(vk::raii::CommandBuffer& commandBuffer)
	{
		commandBuffer.endRendering();
		_renderingActive = false;

		vk::ImageMemoryBarrier2 toPresent{};
		toPresent.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		toPresent.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
		toPresent.dstStageMask = vk::PipelineStageFlagBits2::eNone;
		toPresent.dstAccessMask = vk::AccessFlagBits2::eNone;
		toPresent.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		toPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
		toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toPresent.image = _swapchain.Image(_imageIndex);
		vk::ImageSubresourceRange imgSubresourceRange{};
		imgSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgSubresourceRange.baseMipLevel = 0;
		imgSubresourceRange.levelCount = 1;
		imgSubresourceRange.baseArrayLayer = 0;
		imgSubresourceRange.layerCount = 1;
		toPresent.subresourceRange = imgSubresourceRange;
		vk::DependencyInfo dependencyInfo{};
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &toPresent;
		commandBuffer.pipelineBarrier2(dependencyInfo);
	}
	
}
