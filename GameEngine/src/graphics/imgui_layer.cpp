#include "imgui_layer.hpp"

#include <imgui.h>

#include "graphics_device.hpp"

namespace Ludus::Graphics
{
    ImGuiLayer::~ImGuiLayer() = default;

    bool ImGuiLayer::Initialize(GLFWwindow* window, RendererBackend backend, IGraphicsDevice& device)
    {
        _backend = CreateImGuiBackend(backend);
        if (!_backend)
            return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (_backend->SupportsViewports())
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();

        if (!_backend->Initialize(window, device))
        {
            ImGui::DestroyContext();
            _backend.reset();
            return false;
        }

        _initialized = true;
        return true;
    }

    void ImGuiLayer::Begin()
    {
        _backend->BeginFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        _backend->Render(ImGui::GetDrawData());
    }

    void ImGuiLayer::Shutdown()
    {
        if (_initialized)
        {
            _backend->Shutdown();
            ImGui::DestroyContext();
            _initialized = false;
        }
        _backend.reset();
    }

}
