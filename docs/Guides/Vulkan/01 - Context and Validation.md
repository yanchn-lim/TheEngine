# Context and Validation

[[Vulkan Implementation Tutorial|Back to tutorial index]]

## Goal

`VulkanContext` owns the instance-level objects. It does not select a GPU, create a logical device, own swapchain state, or render a frame.

```cpp
class VulkanContext
{
public:
    bool Init(GLFWwindow* window);
    void Shutdown();

    VkInstance GetInstance() const { return _instance; }
    VkSurfaceKHR GetSurface() const { return _surface; }

private:
    bool CreateInstance();
    bool CreateDebugMessenger();
    bool CreateSurface(GLFWwindow* window);
    bool CheckValidationLayerSupport() const;

    VkInstance _instance{ VK_NULL_HANDLE };
    VkDebugUtilsMessengerEXT _debugMessenger{ VK_NULL_HANDLE };
    VkSurfaceKHR _surface{ VK_NULL_HANDLE };
};
```

## Initialization Order

`Init` should only orchestrate the owned stages. Every failure calls `Shutdown()` so a partially initialized context is safe to discard.

```cpp
bool VulkanContext::Init(GLFWwindow* window)
{
    if (window == nullptr || glfwVulkanSupported() != GLFW_TRUE)
    {
        return false;
    }

    if (!CreateInstance() || !CreateDebugMessenger() || !CreateSurface(window))
    {
        Shutdown();
        return false;
    }

    return true;
}
```

GLFW supplies the platform surface extensions. Do not guess Windows, Android, or Web-specific instance extensions yourself:

```cpp
uint32_t extensionCount = 0;
const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);
#if defined(_DEBUG)
extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
```

Set `VkApplicationInfo::apiVersion` to `VK_API_VERSION_1_3`. Populate `VkInstanceCreateInfo` with those extensions and, in Debug, the `VK_LAYER_KHRONOS_validation` layer name.

## Debug Messenger

The callback must be free or static because Vulkan stores a C function pointer. Keep it diagnostic only: log the message and return `VK_FALSE`.

```cpp
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    ENGINE_ERROR("Vulkan validation: {}", callbackData->pMessage);
    return VK_FALSE;
}
```

`vkCreateDebugUtilsMessengerEXT` is an extension function, so load it through `vkGetInstanceProcAddr`. Store the matching destroy function or load it again during shutdown. Do not call extension functions directly unless your headers and loader path guarantee it.

## Shutdown

Destroy in reverse dependency order:

```cpp
void VulkanContext::Shutdown()
{
    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }

    DestroyDebugMessenger(_instance, _debugMessenger);
    _debugMessenger = VK_NULL_HANDLE;

    if (_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}
```

Guard the debug messenger destroy helper against a null instance or messenger. Deleting copy construction and copy assignment prevents two objects from destroying the same handles.

## Verify

Run the Vulkan backend with no device yet. You should get a window and no validation messages on startup or shutdown. If the validation layer cannot be found, confirm the SDK is installed and run the instance-layer enumeration before creation.

Next: [[02 - Physical Device and Queues]].
