#pragma once

#include <vulkan/vulkan.h>

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
        void Shutdown();

        VkInstance GetInstance() const { return _instance; }
        VkSurfaceKHR GetSurface() const { return _surface; }
    private:
        bool CheckValidationLayerSupport() const;
        bool CreateInstance();
        bool CreateDebugMessenger();
        bool CreateSurface(GLFWwindow* window);

        VkInstance _instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT _debugMessenger{ VK_NULL_HANDLE };
        VkSurfaceKHR _surface{ VK_NULL_HANDLE };
	};
}
