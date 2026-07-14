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

        //log validation messages
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void*)
        {
            const char* message = callbackData && callbackData->pMessage
                ? callbackData->pMessage
                : "Unknown Vulkan validation message";

            if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                Debug::LogError("Vulkan validation: ", message);
            else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                Debug::LogWarning("Vulkan validation: ", message);
            else
                Debug::LogVerbose("Vulkan validation: ", message);

            return VK_FALSE;
        }

        //setup debug config
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
        {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = DebugCallback;
        }

        //destroy debug messenger
        void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
        {
            const auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (destroy)
                destroy(instance, debugMessenger, nullptr);
        }
    }

    //cleanup context
    VulkanContext::~VulkanContext()
    {
        Shutdown();
    }

    //initialize context
    bool VulkanContext::Init(GLFWwindow* window)
    {
        //reset previous state
        Shutdown();

        if (!window)
        {
            Debug::LogError("VulkanContext::Init failed: window is null");
            return false;
        }

        if (glfwVulkanSupported() != GLFW_TRUE)
        {
            Debug::LogError("VulkanContext::Init failed: Vulkan is not supported by GLFW");
            return false;
        }

#if defined(_DEBUG)
        //check validation layer
        if (!CheckValidationLayerSupport())
        {
            Debug::LogError("VulkanContext::Init failed: VK_LAYER_KHRONOS_validation is unavailable");
            return false;
        }
#endif

        //create context resources
        if (!CreateInstance())
        {
            Shutdown();
            return false;
        }

#if defined(_DEBUG)
        if (!CreateDebugMessenger())
        {
            Shutdown();
            return false;
        }
#endif

        if (!CreateSurface(window))
        {
            Shutdown();
            return false;
        }

        return true;
    }

    //destroy context resources
    void VulkanContext::Shutdown()
    {
        //destroy surface
        if (_surface != VK_NULL_HANDLE && _instance != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(_instance, _surface, nullptr);
            _surface = VK_NULL_HANDLE;
        }

        //destroy debug messenger
        if (_debugMessenger != VK_NULL_HANDLE && _instance != VK_NULL_HANDLE)
        {
            DestroyDebugMessenger(_instance, _debugMessenger);
            _debugMessenger = VK_NULL_HANDLE;
        }

        //destroy instance
        if (_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(_instance, nullptr);
            _instance = VK_NULL_HANDLE;
        }
    }

    //check validation layer support
    bool VulkanContext::CheckValidationLayerSupport() const
    {
        uint32_t layerCount = 0;

        //get layer count
        VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (result != VK_SUCCESS)
        {
            Debug::LogError("Failed to enumerate Vulkan instance layer count");
            return false;
        }

        //get layers
        std::vector<VkLayerProperties> availableLayers(layerCount);
        result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        if (result != VK_SUCCESS)
        {
            Debug::LogError("Failed to enumerate Vulkan instance layers");
            return false;
        }

        //find validation layer
        for (const VkLayerProperties& layer : availableLayers)
        {
            if (strcmp(layer.layerName, VALIDATION_LAYER) == 0)
                return true;
        }

        return false;
    }

    //create vulkan instance
    bool VulkanContext::CreateInstance()
    {
        //get glfw extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions || glfwExtensionCount == 0)
        {
            Debug::LogError("VulkanContext::CreateInstance failed: GLFW returned no required extensions");
            return false;
        }

        //copy extensions
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if defined(_DEBUG)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        //setup app info
        VkApplicationInfo applicationInfo{};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pApplicationName = "TheEngine";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.pEngineName = "TheEngine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_3;

        //setup instance info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &applicationInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

#if defined(_DEBUG)
        constexpr const char* validationLayers[] = { VALIDATION_LAYER };
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;

        //setup creation validation
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
#endif

        //create instance
        const VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
        if (result != VK_SUCCESS)
        {
            Debug::LogError(
                "VulkanContext::CreateInstance failed with VkResult ",
                static_cast<int>(result));
            return false;
        }

        return true;
    }

    //create debug messenger
    bool VulkanContext::CreateDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        PopulateDebugMessengerCreateInfo(createInfo);

        //load extension function
        const auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!create)
        {
            Debug::LogError("VulkanContext::CreateDebugMessenger failed: extension function is unavailable");
            return false;
        }

        const VkResult result = create(_instance, &createInfo, nullptr, &_debugMessenger);
        if (result != VK_SUCCESS)
        {
            Debug::LogError(
                "VulkanContext::CreateDebugMessenger failed with VkResult ",
                static_cast<int>(result));
            return false;
        }

        return true;
    }

    //create glfw surface
    bool VulkanContext::CreateSurface(GLFWwindow* window)
    {
        const VkResult result = glfwCreateWindowSurface(_instance, window, nullptr, &_surface);
        if (result != VK_SUCCESS)
        {
            Debug::LogError(
                "VulkanContext::CreateSurface failed with VkResult ",
                static_cast<int>(result));
            return false;
        }

        return true;
    }
}
