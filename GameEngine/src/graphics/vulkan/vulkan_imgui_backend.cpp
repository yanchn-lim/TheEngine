#include "vulkan_imgui_backend.hpp"

#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "debug/debug.hpp"
#include "vulkan_graphics_device.hpp"

namespace
{
    // adapt ImGui's native callback to the engine logger
    void CheckVulkanResult(VkResult result)
    {
        if (result < 0) Debug::LogError("ImGui Vulkan error: ", static_cast<int>(result));
    }
}

namespace Graphics
{
    bool VulkanImGuiBackend::Initialize(GLFWwindow* window, IGraphicsDevice& device)
    {
        // obtain the native Vulkan objects required by the official ImGui back end
        _device = dynamic_cast<VulkanGraphicsDevice*>(&device);
        if (!_device || !window) return false;

        _glfwInitialized = ImGui_ImplGlfw_InitForVulkan(window, true);
        if (!_glfwInitialized) return false;

        _colorFormat = static_cast<VkFormat>(_device->SwapchainFormat());
        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_3;
        info.Instance = static_cast<VkInstance>(_device->NativeInstance());
        info.PhysicalDevice = static_cast<VkPhysicalDevice>(_device->NativePhysicalDevice());
        info.Device = static_cast<VkDevice>(_device->NativeDevice());
        info.QueueFamily = _device->NativeGraphicsQueueFamily();
        info.Queue = static_cast<VkQueue>(_device->NativeGraphicsQueue());
        info.DescriptorPoolSize = 64;
        info.MinImageCount = 2;
        info.ImageCount = _device->SwapchainImageCount();
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.UseDynamicRendering = true;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_colorFormat;
        info.CheckVkResultFn = CheckVulkanResult;
        info.MinAllocationSize = 1024 * 1024;

        _rendererInitialized = ImGui_ImplVulkan_Init(&info);
        if (!_rendererInitialized)
        {
            ImGui_ImplGlfw_Shutdown();
            _glfwInitialized = false;
            _device = nullptr;
        }
        return _rendererInitialized;
    }

    void VulkanImGuiBackend::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    void VulkanImGuiBackend::Render(ImDrawData* drawData)
    {
        // append ImGui commands to the render pass already opened by Renderer
        if (_rendererInitialized && drawData)
            ImGui_ImplVulkan_RenderDrawData(drawData,
                static_cast<VkCommandBuffer>(_device->ActiveCommandBuffer()));
    }

    void VulkanImGuiBackend::Shutdown()
    {
        // wait before destroying resources that submitted ImGui commands can reference
        if (_device) _device->WaitIdle();
        if (_rendererInitialized) ImGui_ImplVulkan_Shutdown();
        if (_glfwInitialized) ImGui_ImplGlfw_Shutdown();
        _rendererInitialized = false;
        _glfwInitialized = false;
        _device = nullptr;
        _colorFormat = VK_FORMAT_UNDEFINED;
    }
}
