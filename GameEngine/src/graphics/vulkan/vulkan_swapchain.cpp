#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"

#include "debug/debug.hpp"

namespace Graphics
{
	bool VulkanSwapchain::Create(const VulkanDevice& device, vk::SurfaceKHR surface, vk::Extent2D requestedExtent)
	{
		Shutdown();

		try
		{
			const auto capabilities = device.PhysicalDevice().getSurfaceCapabilitiesKHR(surface);
			const auto formats = device.PhysicalDevice().getSurfaceFormatsKHR(surface);
			const auto presentModes = device.PhysicalDevice().getSurfacePresentModesKHR(surface);

			if (formats.empty() || presentModes.empty())
			{
				Debug::LogError("VulkanSwapchain::Create : Surface has no formats or present modes");
				return false;
			}

			//get format with preference
			vk::SurfaceFormatKHR selectedFormat = formats.front();
			for (const vk::SurfaceFormatKHR& available : formats)
			{
				if (available.format == vk::Format::eB8G8R8A8Srgb &&
					available.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					selectedFormat = available;
					break;
				}
			}

			//get present mode
			vk::PresentModeKHR selectedPresentMode = vk::PresentModeKHR::eFifo;
			for (vk::PresentModeKHR available : presentModes)
			{
				if (available == vk::PresentModeKHR::eMailbox)
				{
					selectedPresentMode = available;
					break;
				}
			}

			//get size of framebuffer
			vk::Extent2D selectedExtent;
			if (capabilities.currentExtent.width != UINT32_MAX)
			{
				selectedExtent = capabilities.currentExtent;
			}
			else
			{
				selectedExtent.width = std::clamp(
					requestedExtent.width,
					capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width);

				selectedExtent.height = std::clamp(
					requestedExtent.height,
					capabilities.minImageExtent.height,
					capabilities.maxImageExtent.height);
			}

			if (selectedExtent.width == 0 || selectedExtent.height == 0)
			{
				Debug::LogWarning(
					"VulkanSwapchain::Create: framebuffer extent is zero");
				return false;
			}

			//get one more image than minimum
			uint32_t imageCount = capabilities.minImageCount + 1;
			if (capabilities.maxImageCount > 0 &&
				imageCount > capabilities.maxImageCount)
			{
				imageCount = capabilities.maxImageCount;
			}

			if (!(capabilities.supportedUsageFlags &
				vk::ImageUsageFlagBits::eColorAttachment))
			{
				Debug::LogError(
					"VulkanSwapchain::Create: color attachments aren't supported");
				return false;
			}

			constexpr std::array compositeAlphaCandidates{
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
				vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
				vk::CompositeAlphaFlagBitsKHR::eInherit
			};

			//get composite mode (blending)
			vk::CompositeAlphaFlagBitsKHR selectedCompositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			bool foundCompositeAlpha = false;

			for (vk::CompositeAlphaFlagBitsKHR candidate :
			compositeAlphaCandidates)
			{
				if (capabilities.supportedCompositeAlpha & candidate)
				{
					selectedCompositeAlpha = candidate;
					foundCompositeAlpha = true;
					break;
				}
			}

			if (!foundCompositeAlpha)
			{
				Debug::LogError(
					"VulkanSwapchain::Create: no composite-alpha mode available");
				return false;
			}


			const uint32_t queueFamilies[] = { device.GraphicsQueueFamily(), device.PresentQueueFamily() };
			const bool separateQueues = queueFamilies[0] != queueFamilies[1];

			vk::SwapchainCreateInfoKHR createInfo{};
			createInfo.surface = surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = selectedFormat.format;
			createInfo.imageColorSpace = selectedFormat.colorSpace;
			createInfo.imageExtent = selectedExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
			createInfo.imageSharingMode = separateQueues ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = separateQueues ? 2u : 0u;
			createInfo.pQueueFamilyIndices = separateQueues ? queueFamilies : nullptr;
			createInfo.preTransform = capabilities.currentTransform;
			createInfo.compositeAlpha = selectedCompositeAlpha;
			createInfo.presentMode = selectedPresentMode;
			createInfo.clipped = vk::True;
			createInfo.oldSwapchain = nullptr;

			//create temp owners to prevent failure, leaving swapchain half initialized
			vk::raii::SwapchainKHR newSwapchain(device.Device(), createInfo);
			std::vector<vk::Image> newImages = newSwapchain.getImages();
			std::vector<vk::raii::ImageView> newImageViews;
			newImageViews.reserve(newImages.size());

			for (vk::Image image : newImages)
			{
				vk::ImageViewCreateInfo viewInfo{};
				viewInfo.image = image;
				viewInfo.viewType = vk::ImageViewType::e2D;
				viewInfo.format = selectedFormat.format;
				vk::ImageSubresourceRange subresrange{};
				subresrange.aspectMask = vk::ImageAspectFlagBits::eColor;
				subresrange.baseMipLevel = 0;
				subresrange.levelCount = 1;
				subresrange.baseArrayLayer = 0;
				subresrange.layerCount = 1;
				viewInfo.subresourceRange = subresrange;
				
				newImageViews.emplace_back(device.Device(), viewInfo);
			}

			//swap the temp into actl
			_swapchain = std::move(newSwapchain);
			_images = std::move(newImages);
			_imageViews = std::move(newImageViews);
			_format = selectedFormat.format;
			_extent = selectedExtent;

			return true;
		}
		catch (const std::exception& error)
		{
			Debug::LogError("VulkanSwapchain::Create: ", error.what());
			Shutdown();
			return false;
		}
	}

	bool VulkanSwapchain::Recreate(const VulkanDevice& device, vk::SurfaceKHR surface, vk::Extent2D requestedExtent)
	{
		device.WaitIdle();
		Shutdown();
		return Create(device, surface, requestedExtent);
	}

	void VulkanSwapchain::Shutdown() noexcept
	{
		_imageViews.clear();
		_images.clear();
		_swapchain = nullptr;
		_format = vk::Format::eUndefined;
		_extent = {};
	}

}