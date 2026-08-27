#pragma once

#include "vulkan_include.hpp"
#include <vector>

namespace Ludus::Graphics
{
	class VulkanDevice;

	// owns presentable images and their views for the current window size
	class VulkanSwapchain
	{
	public:
		bool Create(const VulkanDevice& device, vk::SurfaceKHR surface,
			vk::Extent2D requestedExtent, bool vsync);
		void Shutdown() noexcept;
	
		const vk::raii::SwapchainKHR& Swapchain() const { return _swapchain; }
		vk::Format Format() const { return _format; }
		vk::Extent2D Extent() const { return _extent; }
		vk::Image Image(uint32_t index) const { return _images.at(index); }
		vk::ImageView ImageView(uint32_t index) const { return *_imageViews.at(index); }
		uint32_t ImageCount() const { return static_cast<uint32_t>(_images.size()); }
	
	private:
		vk::raii::SwapchainKHR _swapchain{ nullptr };
		std::vector<vk::Image> _images;
		std::vector<vk::raii::ImageView> _imageViews;
		vk::Format _format = vk::Format::eUndefined;
		vk::Extent2D _extent{};
	};
}
