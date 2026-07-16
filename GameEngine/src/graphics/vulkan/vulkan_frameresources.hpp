#pragma once

#include "vulkan_include.hpp"

namespace Graphics
{
	inline constexpr uint32_t FramesInFlight = 2;

	struct VulkanFrameResources
	{
		vk::raii::CommandPool commandPool{ nullptr };
		vk::raii::CommandBuffer commandBuffer{ nullptr };
		vk::raii::Fence inFlightFence{ nullptr };
		vk::raii::Semaphore imageAvailable{ nullptr };
		vk::raii::Semaphore renderFinished{ nullptr };
	};
}