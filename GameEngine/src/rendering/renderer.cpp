#include "renderer.hpp"

namespace Rendering
{
    Renderer::Renderer(Graphics::IGraphicsDevice& device, const Assets::AssetManager& assets)
        : _device(device), _resources(assets, device)
    {
    }

    void Renderer::Submit(MeshInstanceDesc item)
    {
        _items.push_back(std::move(item));
    }

    Graphics::FrameStatus Renderer::Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height)
    {
        // begin the selected backend frame before collecting render data
        const Graphics::FrameStatus status = _device.BeginFrame(_frame);
        _frameReady = status == Graphics::FrameStatus::Success;
        if (!_frameReady)
        {
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

        for (const MeshInstanceDesc& item : _items)
            Draw(commands, item, frameConstants);

        return Graphics::FrameStatus::Success;
    }

    Graphics::FrameStatus Renderer::EndFrame()
    {
        // UI is recorded before this call so Vulkan can share the active render pass
        if (!_frameReady) return Graphics::FrameStatus::Skip;
        _device.GetCommandList(_frame).EndRenderPass();
        _items.clear();

        Graphics::FrameStatus status = _device.EndFrame(_frame);
        if (status == Graphics::FrameStatus::Success)
        {
            ++_frame.frameNumber;
            status = _device.Present(_frame);
        }

        _frameReady = false;
        return status;
    }

    void Renderer::OnResize(uint32_t width, uint32_t height) { _device.OnResize(width, height); }

    void Renderer::Shutdown()
    {
        _resources.Clear();
        _items.clear();
        _frameReady = false;
    }

    void Renderer::Draw(Graphics::IGraphicsCommandList& commands, const MeshInstanceDesc& item,
        const Graphics::FrameConstants& frameConstants)
    {
        ResolvedDraw draw;
        if (!_resources.Resolve(item.mesh, item.material, draw))
            return;

        Graphics::DrawConstants constants;
        constants.model = item.transform;

        // emit the same command order for OpenGL and Vulkan
        commands.SetPipeline(draw.pipeline);
        commands.SetVertexBuffer(draw.vertexBuffer, draw.layout);
        if (draw.indexBuffer) commands.SetIndexBuffer(draw.indexBuffer, Graphics::IndexFormat::UInt32);
        commands.SetFrameConstants(frameConstants);
        commands.SetMaterialResources(draw.texture, draw.sampler);
        commands.SetDrawConstants(constants);
        if (draw.indexCount) commands.DrawIndexed(draw.indexCount);
        else commands.Draw(draw.vertexCount);
    }
}
