#pragma once

#include "graphics/imgui_backend.hpp"

namespace Graphics
{
    // adapts ImGui platform and renderer calls to the active OpenGL context
    class OpenGLImGuiBackend final : public IImGuiBackend
    {
    public:
        bool IsAvailable() const override { return true; }
        bool SupportsViewports() const override { return true; }
        bool Initialize(GLFWwindow* window, IGraphicsDevice& device) override;
        void BeginFrame() override;
        void Render(ImDrawData* drawData) override;
        void Shutdown() override;
    };
}
