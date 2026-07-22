#include "vulkan_context.hpp"

#include <cstring>
#include <vector>

#include <GLFW/glfw3.h>

#include "debug/debug.hpp"

namespace Graphics
{
    namespace
    {
        constexpr const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

        // route Vulkan validation messages through the engine logger
        VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT,
            const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
            void*)
        {
            const char* message =
                callbackData && callbackData->pMessage
                ? callbackData->pMessage
                : "Unknown Vulkan validation message";

            if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            {
                Debug::LogError("Vulkan validation: ", message);
            }
            else if (severity &
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            {
                Debug::LogWarning("Vulkan validation: ", message);
            }
            else
            {
                Debug::LogVerbose("Vulkan validation: ", message);
            }

            return vk::False;
        }

        vk::DebugUtilsMessengerCreateInfoEXT MakeDebugMessengerInfo()
        {
            using messageSeverityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT;
            using messageTypeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT;

            vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.messageSeverity =
                messageSeverityFlags::eWarning |
                messageSeverityFlags::eError;
            createInfo.messageType =
                messageTypeFlags::eGeneral |
                messageTypeFlags::eValidation |
				messageTypeFlags::ePerformance;
            createInfo.pfnUserCallback = DebugCallback;
			return createInfo;
        }
    }

    // release context owners if explicit shutdown was skipped
    VulkanContext::~VulkanContext()
    {
        Shutdown();
    }

    // initialize instance-level Vulkan state for the supplied window
    bool VulkanContext::Init(GLFWwindow* window)
    {
        // reset previous state so repeated initialization remains safe
        Shutdown();

        if (!window || glfwVulkanSupported() != GLFW_TRUE)
        {
            Debug::LogError("VulkanContext::Init : Vulkan or GLFW window is unavailable");
            return false;
        }

        try
        {
#if defined(_DEBUG)
            // require the validation layer in debug builds
            if (!ValidationLayerAvailable())
                throw std::runtime_error("VK_LAYER_KHRONOS_validation is unavailable");
            
#endif
            CreateInstance();
            CreateDebugMessenger();
            CreateSurface(window);
            return true;
        }
        catch (const vk::SystemError& error)
        {
            Debug::LogError("VulkanContext::Init : ", error.what());
        }
        catch (const std::exception& error)
        {
            Debug::LogError("VulkanContext::Init : ", error.what());
        }

        Shutdown();
        return false;
    }

    // destroy owners in surface-to-instance dependency order
    void VulkanContext::Shutdown() noexcept
    {
		_surface = nullptr;
		_debugMessenger = nullptr;
		_instance = nullptr;
    }

    bool VulkanContext::ValidationLayerAvailable() const
    {
        for (auto& layer : _loader.enumerateInstanceLayerProperties())
        {
            if (std::strcmp(layer.layerName, VALIDATION_LAYER) == 0)
                return true;
        }

        return false;
    }

    // create the Vulkan instance with GLFW and debug extensions
    void VulkanContext::CreateInstance()
    {
        // request every platform extension required by GLFW
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions || glfwExtensionCount == 0)
            throw std::runtime_error("GLFW returned no Vulkan instance extensions");

        // keep extension and layer storage alive through instance creation
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        std::vector<const char*> layers;

#if defined(_DEBUG)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        layers.push_back(VALIDATION_LAYER);
#endif

        vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "TheEngine";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		appInfo.pEngineName = "TheEngine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = MakeDebugMessengerInfo();
        vk::InstanceCreateInfo createInfo;
#if defined(_DEBUG)
		createInfo.pNext = &debugCreateInfo;
#endif
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

		_instance = vk::raii::Instance(_loader, createInfo);
    }

    // create the debug messenger only when validation is compiled in
    void VulkanContext::CreateDebugMessenger()
    {
#if defined(_DEBUG)
        _debugMessenger = vk::raii::DebugUtilsMessengerEXT(
            _instance,
            MakeDebugMessengerInfo());
#endif
    }

    // wrap the native GLFW surface in a Vulkan RAII owner
    void VulkanContext::CreateSurface(GLFWwindow* window)
    {
		VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
		const VkResult result = glfwCreateWindowSurface(
            static_cast<VkInstance>(*_instance), 
            window, 
            nullptr, 
            &rawSurface);

        if (result != VK_SUCCESS)
            throw std::runtime_error("glfwCreateWindowSurface failed");

        _surface = vk::raii::SurfaceKHR(_instance, rawSurface);
    }
}
