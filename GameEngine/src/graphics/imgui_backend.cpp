#include "imgui_backend.hpp"

#include "opengl/opengl_imgui_backend.hpp"
#include "vulkan/vulkan_imgui_backend.hpp"

namespace Ludus::Graphics
{
    std::unique_ptr<IImGuiBackend> CreateImGuiBackend(RendererBackend backend)
    {
        // pair ImGui with the same back end chosen for the graphics device
        if (backend == RendererBackend::OPENGL) return std::make_unique<OpenGLImGuiBackend>();
        if (backend == RendererBackend::VULKAN) return std::make_unique<VulkanImGuiBackend>();
        return nullptr;
    }
}
