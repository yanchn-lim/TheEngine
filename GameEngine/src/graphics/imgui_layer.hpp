#pragma once

#include <memory>

#include <imgui.h>

#include "imgui_backend.hpp"
#include "renderer_backend.hpp"

struct GLFWwindow;

namespace Ludus::Graphics
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
		ImTextureID AddTexture(GpuTextureHandle texture, GpuSamplerHandle sampler);
		void RemoveTexture(ImTextureID texture);
        void Shutdown();

    private:
        bool _initialized = false;
        std::unique_ptr<IImGuiBackend> _backend;
    };
}
