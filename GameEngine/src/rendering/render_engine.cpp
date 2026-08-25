#include "render_engine.hpp"

#include "graphics/graphics_device_factory.hpp"

namespace Rendering
{
    std::unique_ptr<RenderEngine> RenderEngine::Create(
        Graphics::RendererBackend backend,
        const Graphics::GraphicsDeviceDesc& deviceDesc,
        const Assets::AssetManager& assets)
    {
        std::unique_ptr<Graphics::IGraphicsDevice> device =
            Graphics::CreateGraphicsDevice(backend);

        if (!device)
            return nullptr;

        if (!device->Initialize(deviceDesc))
        {
            device->Shutdown();
            return nullptr;
        }

        std::unique_ptr<RenderEngine> renderEngine =
            std::make_unique<RenderEngine>(
                std::move(device),
                assets);

        if (!renderEngine->_imgui.Initialize(
            static_cast<GLFWwindow*>(deviceDesc.window),
            backend,
            *renderEngine->_device))
        {
            renderEngine->Shutdown();
            return nullptr;
        }

        return renderEngine;
    }

    RenderEngine::RenderEngine(
        std::unique_ptr<Graphics::IGraphicsDevice> device,
        const Assets::AssetManager& assets)
        : _device(std::move(device)), _resources(assets, *_device)
    {
    }

    void RenderEngine::BeginImGui()
    {
        _imgui.Begin();
    }

    void RenderEngine::EndImGui()
    {
        _imgui.End();
    }

    void RenderEngine::Submit(MeshInstanceDesc item)
    {
        _items.push_back(std::move(item));
    }

    Graphics::FrameStatus RenderEngine::Render(const Graphics::Camera2D& camera, uint32_t width, uint32_t height)
    {
        // begin the selected backend frame before collecting render data
        const Graphics::FrameStatus status = _device->BeginFrame();
        if (status != Graphics::FrameStatus::Success)
        {
            _items.clear();
            return status;
        }

        _device->BeginRenderPass({});
        _device->SetViewport({ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) });

        Graphics::FrameConstants frameConstants;
        frameConstants.view = camera.GetView();
        frameConstants.projection = camera.GetProjection();

        for (const MeshInstanceDesc& item : _items)
            Draw(item, frameConstants);

        return Graphics::FrameStatus::Success;
    }

    Graphics::FrameStatus RenderEngine::EndFrame()
    {
        // UI is recorded before this call so Vulkan can share the active render pass
        _device->EndRenderPass();
        _items.clear();

        return _device->EndFrame();
    }

    void RenderEngine::OnResize(uint32_t width, uint32_t height) { _device->OnResize(width, height); }

    void RenderEngine::Shutdown()
    {
        _imgui.Shutdown();
        _resources.Clear();
        _items.clear();
        _device->Shutdown();
    }

    void RenderEngine::Draw(const MeshInstanceDesc& item, const Graphics::FrameConstants& frameConstants)
    {
        ResolvedDraw draw;
        if (!_resources.Resolve(item.mesh, item.material, draw))
            return;

        Graphics::DrawConstants constants;
        constants.model = item.transform;

        // emit the same command order for OpenGL and Vulkan
        _device->SetPipeline(draw.pipeline);
        _device->SetVertexBuffer(draw.vertexBuffer, draw.layout);

        if (draw.indexBuffer) 
            _device->SetIndexBuffer(draw.indexBuffer);

        _device->SetFrameConstants(frameConstants);
        _device->SetMaterialResources(draw.texture, draw.sampler);
        _device->SetDrawConstants(constants);

        if (draw.indexCount) 
            _device->DrawIndexed(draw.indexCount);
        else 
            _device->Draw(draw.vertexCount);
    }
}
