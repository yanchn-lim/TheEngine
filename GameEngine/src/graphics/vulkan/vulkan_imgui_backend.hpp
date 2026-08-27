#pragma once

#include "graphics/imgui_backend.hpp"
#include "vulkan_include.hpp"

namespace Ludus::Graphics
{
    class VulkanGraphicsDevice;

    // records ImGui data into the active Vulkan dynamic-rendering pass
    class VulkanImGuiBackend final : public IImGuiBackend
    {
    public:
        bool SupportsViewports() const override { return false; }
        bool Initialize(GLFWwindow* window, IGraphicsDevice& device) override;
        void BeginFrame() override;
        void Render(ImDrawData* drawData) override;
        void Shutdown() override;

    private:
        VulkanGraphicsDevice* _device = nullptr;
        VkFormat _colorFormat = VK_FORMAT_UNDEFINED;
        VkFormat _depthFormat = VK_FORMAT_UNDEFINED;
        bool _glfwInitialized = false;
        bool _rendererInitialized = false;
    };
}
