#pragma once

#include "vulkan_include.hpp"

struct GLFWwindow;

namespace Graphics
{
	// owns the Vulkan instance, validation messenger, and window surface
	class VulkanContext
	{
	public:
        VulkanContext() = default;

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        bool Init(GLFWwindow* window);
        void Shutdown() noexcept;

        const vk::raii::Instance& Instance() const { return _instance; }
        vk::SurfaceKHR SurfaceHandle() const { return *_surface; }
    private:
        // construction is split so each Vulkan dependency is created in order
        bool ValidationLayerAvailable() const;
        void CreateInstance();
        void CreateDebugMessenger();
        void CreateSurface(GLFWwindow* window);

        vk::raii::Context _loader;
        vk::raii::Instance _instance{ nullptr };
		vk::raii::DebugUtilsMessengerEXT _debugMessenger{ nullptr };
		vk::raii::SurfaceKHR _surface{ nullptr };
	};
}
