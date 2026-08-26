#pragma once

#include <memory>
#include <vector>

#include "graphics/camera.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/imgui_layer.hpp"
#include "graphics/renderer_backend.hpp"
#include "render_resource_manager.hpp"
#include "render_item.hpp"

namespace Rendering
{
    class RenderEngine
    {
    public:
        static std::unique_ptr<RenderEngine> Create(
            Graphics::RendererBackend backend,
            const Graphics::GraphicsDeviceDesc& deviceDesc,
            const Assets::AssetManager& assets);

        RenderEngine(std::unique_ptr<Graphics::IGraphicsDevice> device, const Assets::AssetManager& assets);
        ~RenderEngine();
        void BeginImGui();
        void EndImGui();
        void Submit(MeshInstanceDesc item);
        // Render records world and scene draws while EndFrame waits for UI recording
        Graphics::FrameStatus Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height);
        Graphics::FrameStatus EndFrame();
        void OnResize(uint32_t width, uint32_t height);
        void Shutdown();

    private:
        std::unique_ptr<Graphics::IGraphicsDevice> _device;
        // resource resolution and frame-local items stay private to the renderer
        RenderResourceManager _resources;
        Graphics::ImGuiLayer _imgui;
        std::vector<MeshInstanceDesc> _items;
        bool _shutdown = false;

        void Draw(const MeshInstanceDesc& item, const Graphics::FrameConstants& frameConstants);
    };
}
