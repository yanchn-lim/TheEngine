#pragma once

#include "vulkan_include.hpp"

struct GLFWwindow;

namespace Graphics
{
	class VulkanContext
	{
	public:
        VulkanContext() = default;
        ~VulkanContext();

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        bool Init(GLFWwindow* window);
        void Shutdown() noexcept;

        const vk::raii::Instance& Instance() const { return _instance; }
        vk::Instance InstanceHandle() { return *_instance;  }
        const vk::SurfaceKHR SurfaceHandle() const { return *_surface; }
		vk::SurfaceKHR SurfaceHandle() { return *_surface; }
    private:
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
