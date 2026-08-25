#pragma once

#include <vector>

#include "graphics/camera.hpp"
#include "graphics/graphics_device.hpp"
#include "render_resource_manager.hpp"
#include "render_item.hpp"

namespace Rendering
{
    class Renderer
    {
    public:
        Renderer(Graphics::IGraphicsDevice& device, const Assets::AssetManager& assets);
        void Submit(MeshInstanceDesc item);
        // Render records world and scene draws while EndFrame waits for UI recording
        Graphics::FrameStatus Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height);
        Graphics::FrameStatus EndFrame();
        void OnResize(uint32_t width, uint32_t height);
        void Shutdown();

    private:
        Graphics::IGraphicsDevice& _device;
        // resource resolution and frame-local items stay private to the renderer
        RenderResourceManager _resources;
        Graphics::FrameContext _frame;
        std::vector<MeshInstanceDesc> _items;
        bool _frameReady = false;

        void Draw(Graphics::IGraphicsCommandList& commands, const MeshInstanceDesc& item,
            const Graphics::FrameConstants& frameConstants);
    };
}
