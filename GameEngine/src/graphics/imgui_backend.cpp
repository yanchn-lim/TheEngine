#include "imgui_backend.hpp"

#include "opengl/opengl_imgui_backend.hpp"
#include "vulkan/vulkan_imgui_backend.hpp"

namespace Graphics
{
    namespace
    {
        // keeps editor startup valid when no supported renderer back end is selected
        class UnavailableImGuiBackend final : public IImGuiBackend
        {
        public:
            bool IsAvailable() const override { return false; }
            bool SupportsViewports() const override { return false; }
            bool Initialize(GLFWwindow*, IGraphicsDevice&) override { return true; }
            void BeginFrame() override {}
            void Render(ImDrawData*) override {}
            void Shutdown() override {}
        };
    }

    std::unique_ptr<IImGuiBackend> CreateImGuiBackend(RendererBackend backend)
    {
        // pair ImGui with the same back end chosen for the graphics device
        if (backend == RendererBackend::OPENGL) return std::make_unique<OpenGLImGuiBackend>();
        if (backend == RendererBackend::VULKAN) return std::make_unique<VulkanImGuiBackend>();
        return std::make_unique<UnavailableImGuiBackend>();
    }
}
