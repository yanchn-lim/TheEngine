#pragma once

#include "vulkan_include.hpp"

namespace Graphics
{
	// two slots allow CPU recording to overlap work already submitted to the GPU
	inline constexpr uint32_t FramesInFlight = 2;

	// owns synchronization and recording state reused for one frame slot
	struct VulkanFrameResources
	{
		vk::raii::CommandPool commandPool{ nullptr };
		vk::raii::CommandBuffer commandBuffer{ nullptr };
		vk::raii::Fence inFlightFence{ nullptr };
		vk::raii::Semaphore imageAvailable{ nullptr };
	};
}
