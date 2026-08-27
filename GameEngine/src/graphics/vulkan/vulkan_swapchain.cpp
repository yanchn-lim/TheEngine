#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"

#include "debug/debug.hpp"

namespace Ludus::Graphics
{
	bool VulkanSwapchain::Create(const VulkanDevice& device, vk::SurfaceKHR surface,
		vk::Extent2D requestedExtent, bool vsync)
	{
		Shutdown();

		try
		{
			const auto capabilities = device.PhysicalDevice().getSurfaceCapabilitiesKHR(surface);
			const auto formats = device.PhysicalDevice().getSurfaceFormatsKHR(surface);
			const auto presentModes = device.PhysicalDevice().getSurfacePresentModesKHR(surface);

			if (formats.empty() || presentModes.empty())
			{
				Ludus::Debug::LogError("VulkanSwapchain::Create : Surface has no formats or present modes");
				return false;
			}

			// the engine writes display-ready colors on both Vulkan and OpenGL
			// use an UNORM swapchain so presentation does not apply another sRGB encoding pass
			vk::SurfaceFormatKHR selectedFormat = formats.front();
			auto selectFormat = [&formats, &selectedFormat](vk::Format preferred)
			{
				for (const vk::SurfaceFormatKHR& available : formats)
				{
					if (available.format == preferred &&
						available.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
					{
						selectedFormat = available;
						return true;
					}
				}
				return false;
			};

			if (!selectFormat(vk::Format::eB8G8R8A8Unorm))
				selectFormat(vk::Format::eR8G8B8A8Unorm);

			// FIFO provides vsync; otherwise prefer mailbox, then immediate presentation
			vk::PresentModeKHR selectedPresentMode = vk::PresentModeKHR::eFifo;
			if (!vsync)
			{
				bool immediateAvailable = false;
				for (vk::PresentModeKHR available : presentModes)
				{
					if (available == vk::PresentModeKHR::eMailbox)
					{
						selectedPresentMode = available;
						break;
					}

					if (available == vk::PresentModeKHR::eImmediate)
						immediateAvailable = true;
				}

				if (selectedPresentMode != vk::PresentModeKHR::eMailbox &&
					immediateAvailable)
				{
					selectedPresentMode = vk::PresentModeKHR::eImmediate;
				}
			}

			// clamp the requested framebuffer size to the surface limits
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
				Ludus::Debug::LogWarning(
					"VulkanSwapchain::Create: framebuffer extent is zero");
				return false;
			}

			// request one image beyond the minimum when the surface permits it
			uint32_t imageCount = capabilities.minImageCount + 1;
			if (capabilities.maxImageCount > 0 &&
				imageCount > capabilities.maxImageCount)
			{
				imageCount = capabilities.maxImageCount;
			}

			if (!(capabilities.supportedUsageFlags &
				vk::ImageUsageFlagBits::eColorAttachment))
			{
				Ludus::Debug::LogError(
					"VulkanSwapchain::Create: color attachments aren't supported");
				return false;
			}

			constexpr std::array compositeAlphaCandidates{
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
				vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
				vk::CompositeAlphaFlagBitsKHR::eInherit
			};

			// choose the first supported window-system composite mode
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
				Ludus::Debug::LogError(
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

			// keep new owners temporary so failure cannot leave a partial swapchain
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

			// commit the complete replacement only after all image views succeed
			_swapchain = std::move(newSwapchain);
			_images = std::move(newImages);
			_imageViews = std::move(newImageViews);
			_format = selectedFormat.format;
			_extent = selectedExtent;

			return true;
		}
		catch (const std::exception& error)
		{
			Ludus::Debug::LogError("VulkanSwapchain::Create: ", error.what());
			Shutdown();
			return false;
		}
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
