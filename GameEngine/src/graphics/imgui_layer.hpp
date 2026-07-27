#pragma once

#include <memory>

#include "imgui_backend.hpp"
#include "renderer_backend.hpp"

struct GLFWwindow;

namespace Graphics
{
    class IGraphicsDevice;

    class ImGuiLayer
    {
    public:
        ImGuiLayer() = default;
        ~ImGuiLayer();

        ImGuiLayer(const ImGuiLayer&) = delete;
        ImGuiLayer& operator=(const ImGuiLayer&) = delete;

        bool Initialize(GLFWwindow* window, RendererBackend backend, IGraphicsDevice& device);
        void Begin();
        void End();
        void Shutdown();
        bool IsInitialized() const noexcept;

    private:
        bool _initialized = false;
        std::unique_ptr<IImGuiBackend> _backend;
    };
}
