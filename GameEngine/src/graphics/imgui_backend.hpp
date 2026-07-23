#pragma once

#include <memory>

#include "renderer_backend.hpp"
#include "graphics_device.hpp"

struct GLFWwindow;
struct ImDrawData;

namespace Graphics
{
    // isolates ImGui frame operations from the engine and selected graphics API
    class IImGuiBackend
    {
    public:
        virtual ~IImGuiBackend() = default;
        virtual bool IsAvailable() const = 0;
        virtual bool SupportsViewports() const = 0;
        virtual bool Initialize(GLFWwindow* window, IGraphicsDevice& device) = 0;
        virtual void BeginFrame() = 0;
        virtual void Render(ImDrawData* drawData) = 0;
        virtual void Shutdown() = 0;
    };

    std::unique_ptr<IImGuiBackend> CreateImGuiBackend(RendererBackend backend);
}
