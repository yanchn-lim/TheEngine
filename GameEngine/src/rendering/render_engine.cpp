#include "render_engine.hpp"

#include "graphics/graphics_device_factory.hpp"

namespace Ludus::Rendering
{
    std::unique_ptr<RenderEngine> RenderEngine::Create(
        Ludus::Graphics::RendererBackend backend,
        const Ludus::Graphics::GraphicsDeviceDesc& deviceDesc,
        const Ludus::Assets::AssetManager& assets)
    {
        std::unique_ptr<Ludus::Graphics::IGraphicsDevice> device =
            Ludus::Graphics::CreateGraphicsDevice(backend);

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
        std::unique_ptr<Ludus::Graphics::IGraphicsDevice> device,
        const Ludus::Assets::AssetManager& assets)
        : _device(std::move(device)), _resources(assets, *_device)
    {
    }

    RenderEngine::~RenderEngine()
    {
        Shutdown();
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

    Ludus::Graphics::FrameStatus RenderEngine::Render(const Ludus::Graphics::Camera2D& camera, uint32_t width, uint32_t height)
    {
        // begin the selected backend frame before collecting render data
        const Ludus::Graphics::FrameStatus status = _device->BeginFrame();
        if (status != Ludus::Graphics::FrameStatus::Success)
        {
            _items.clear();
            return status;
        }

        _device->BeginRenderPass({});
        _device->SetViewport({ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) });

        Ludus::Graphics::FrameConstants frameConstants;
        frameConstants.view = camera.GetView();
        frameConstants.projection = camera.GetProjection();

        for (const MeshInstanceDesc& item : _items)
            Draw(item, frameConstants);

        return Ludus::Graphics::FrameStatus::Success;
    }

    Ludus::Graphics::FrameStatus RenderEngine::EndFrame()
    {
        // UI is recorded before this call so Vulkan can share the active render pass
        _device->EndRenderPass();
        _items.clear();

        return _device->EndFrame();
    }

    void RenderEngine::OnResize(uint32_t width, uint32_t height) { _device->OnResize(width, height); }

    void RenderEngine::Shutdown()
    {
        if (_shutdown)
            return;

        _shutdown = true;
        _imgui.Shutdown();
        _resources.Clear();
        _items.clear();
        _device->Shutdown();
    }

    void RenderEngine::Draw(const MeshInstanceDesc& item, const Ludus::Graphics::FrameConstants& frameConstants)
    {
        std::vector<ResolvedDraw> draws;
        if (!_resources.Resolve(item.mesh, item.materialOverride, draws))
            return;

        Ludus::Graphics::DrawConstants constants;
        constants.model = item.transform;

        for (const ResolvedDraw& draw : draws)
        {
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
}
