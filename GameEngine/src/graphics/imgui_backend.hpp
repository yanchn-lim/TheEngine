#pragma once

#include <memory>

#include <imgui.h>

#include "renderer_backend.hpp"
#include "graphics_device.hpp"

struct GLFWwindow;
namespace Ludus::Graphics
{
    // isolates ImGui frame operations from the engine and selected graphics API
    class IImGuiBackend
    {
    public:
        virtual ~IImGuiBackend() = default;
        virtual bool SupportsViewports() const = 0;
        virtual bool Initialize(GLFWwindow* window, IGraphicsDevice& device) = 0;
        virtual void BeginFrame() = 0;
        virtual void Render(ImDrawData* drawData) = 0;
		virtual ImTextureID AddTexture(
			GpuTextureHandle texture,
			GpuSamplerHandle sampler) = 0;
		virtual void RemoveTexture(ImTextureID texture) = 0;
        virtual void Shutdown() = 0;
    };

    std::unique_ptr<IImGuiBackend> CreateImGuiBackend(RendererBackend backend);
}
