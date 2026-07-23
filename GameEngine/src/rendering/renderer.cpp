#include "renderer.hpp"

#include <type_traits>

#include "debug/debug.hpp"

namespace Rendering
{
    Renderer::Renderer(Graphics::IGraphicsDevice& device, const Assets::AssetManager& assets, RenderWorld& world)
        : _device(device), _world(world), _resources(assets, device)
    {
    }

    void Renderer::Configure(const RendererDesc& desc) { _desc = desc; }

    Graphics::FrameStatus Renderer::Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height)
    {
        // begin the selected backend frame before collecting render data
        const Graphics::FrameStatus status = _device.BeginFrame(_frame);
        _frameReady = status == Graphics::FrameStatus::Success;
        if (!_frameReady)
        {
            _world.EndFrame();
            _items.clear();
            return status;
        }

        Graphics::IGraphicsCommandList& commands = _device.GetCommandList(_frame);
        commands.BeginRenderPass({});
        commands.AddDebugMarker("World");
        commands.SetViewport({ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) });
        commands.SetScissor({ 0, 0, width, height });

        Graphics::FrameConstants frameConstants;
        frameConstants.view = camera.GetView();
        frameConstants.projection = camera.GetProjection();

        _world.Collect(DefaultRenderLayer, _items);
        // both mesh and sprite variants resolve into the same draw operation
        for (const RenderItem& item : _items)
        {
            std::visit([&](const auto& value)
            {
                using Type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Type, MeshInstanceDesc>)
                    DrawMesh(commands, value, frameConstants);
                else
                    DrawSprite(commands, value, frameConstants);
            }, item);
        }

        return Graphics::FrameStatus::Success;
    }

    Graphics::FrameStatus Renderer::EndFrame()
    {
        // UI is recorded before this call so Vulkan can share the active render pass
        if (!_frameReady) return Graphics::FrameStatus::Skip;
        _device.GetCommandList(_frame).EndRenderPass();
        const Graphics::FrameStatus status = _device.EndFrame(_frame);
        _world.EndFrame();
        _frameReady = status == Graphics::FrameStatus::Success;
        if (_frameReady) ++_frame.frameNumber;
        return status;
    }

    Graphics::FrameStatus Renderer::Present()
    {
        if (!_frameReady) return Graphics::FrameStatus::Skip;
        const Graphics::FrameStatus status = _device.Present(_frame);
        _frameReady = false;
        return status;
    }

    void Renderer::OnResize(uint32_t width, uint32_t height) { _device.OnResize(width, height); }

    void Renderer::Invalidate(Assets::MeshHandle handle)
    {
        if (_frameReady) { Debug::LogWarning("cannot invalidate graphics resources during an active frame"); return; }
        _resources.Invalidate(handle);
    }

    void Renderer::Invalidate(Assets::TextureHandle handle)
    {
        if (_frameReady) { Debug::LogWarning("cannot invalidate graphics resources during an active frame"); return; }
        _resources.Invalidate(handle);
    }

    void Renderer::Invalidate(Assets::ShaderHandle handle)
    {
        if (_frameReady) { Debug::LogWarning("cannot invalidate graphics resources during an active frame"); return; }
        _resources.Invalidate(handle);
    }

    void Renderer::Invalidate(Assets::MaterialHandle handle)
    {
        if (_frameReady) { Debug::LogWarning("cannot invalidate graphics resources during an active frame"); return; }
        _resources.Invalidate(handle);
    }

    void Renderer::Shutdown()
    {
        _resources.Clear();
        _items.clear();
        _frameReady = false;
    }

    void Renderer::DrawMesh(Graphics::IGraphicsCommandList& commands, const MeshInstanceDesc& item,
        const Graphics::FrameConstants& frameConstants)
    {
        ResolvedDraw draw;
        if (_resources.Resolve(item.mesh, item.material, draw))
        {
            Graphics::DrawConstants constants;
            constants.model = item.transform;
            DrawResolved(commands, draw, constants, frameConstants);
        }
    }

    void Renderer::DrawSprite(Graphics::IGraphicsCommandList& commands, const SpriteInstanceDesc& item,
        const Graphics::FrameConstants& frameConstants)
    {
        // sprites use a shared quad while each item supplies texture, tint, and UV data
        const Assets::MaterialHandle material = item.material ? item.material : _desc.spriteMaterial;
        ResolvedDraw draw;
        if (_resources.Resolve(_desc.spriteMesh, material, draw, item.texture))
        {
            Graphics::DrawConstants constants;
            constants.model = item.transform;
            constants.tint = item.tint;
            constants.uvRect = item.uvRect;
            DrawResolved(commands, draw, constants, frameConstants);
        }
    }

    void Renderer::DrawResolved(Graphics::IGraphicsCommandList& commands, const ResolvedDraw& draw,
        const Graphics::DrawConstants& drawConstants, const Graphics::FrameConstants& frameConstants)
    {
        // emit the same command order for OpenGL and Vulkan
        commands.SetPipeline(draw.pipeline);
        commands.SetVertexBuffer(draw.vertexBuffer, draw.layout);
        if (draw.indexBuffer) commands.SetIndexBuffer(draw.indexBuffer, Graphics::IndexFormat::UInt32);
        commands.SetFrameConstants(frameConstants);
        commands.SetMaterialResources(draw.texture, draw.sampler);
        commands.SetDrawConstants(drawConstants);
        if (draw.indexCount) commands.DrawIndexed(draw.indexCount);
        else commands.Draw(draw.vertexCount);
    }
}
