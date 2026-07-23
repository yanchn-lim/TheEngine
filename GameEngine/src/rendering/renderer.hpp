#pragma once

#include <vector>

#include "graphics/camera.hpp"
#include "graphics/graphics_device.hpp"
#include "render_resource_manager.hpp"
#include "render_world.hpp"

namespace Rendering
{
    // supplies shared resources used when a sprite becomes a mesh draw
    struct RendererDesc
    {
        Assets::MeshHandle spriteMesh;
        Assets::MaterialHandle spriteMaterial;
    };

    class Renderer
    {
    public:
        Renderer(Graphics::IGraphicsDevice& device, const Assets::AssetManager& assets, RenderWorld& world);
        void Configure(const RendererDesc& desc);
        // Render records world and scene draws while EndFrame waits for UI recording
        Graphics::FrameStatus Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height);
        Graphics::FrameStatus EndFrame();
        Graphics::FrameStatus Present();
        void OnResize(uint32_t width, uint32_t height);
        void Invalidate(Assets::MeshHandle handle);
        void Invalidate(Assets::TextureHandle handle);
        void Invalidate(Assets::ShaderHandle handle);
        void Invalidate(Assets::MaterialHandle handle);
        void Shutdown();

    private:
        Graphics::IGraphicsDevice& _device;
        RenderWorld& _world;
        // resource resolution and frame-local items stay private to the renderer
        RenderResourceManager _resources;
        RendererDesc _desc;
        Graphics::FrameContext _frame;
        std::vector<RenderItem> _items;
        bool _frameReady = false;

        void DrawMesh(Graphics::IGraphicsCommandList& commands, const MeshInstanceDesc& item,
            const Graphics::FrameConstants& frameConstants);
        void DrawSprite(Graphics::IGraphicsCommandList& commands, const SpriteInstanceDesc& item,
            const Graphics::FrameConstants& frameConstants);
        void DrawResolved(Graphics::IGraphicsCommandList& commands, const ResolvedDraw& draw,
            const Graphics::DrawConstants& drawConstants, const Graphics::FrameConstants& frameConstants);
    };
}
