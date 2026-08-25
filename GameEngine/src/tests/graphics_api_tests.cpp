#include "graphics_api_tests.hpp"

#include "assets/asset_manager.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/graphics_handles.hpp"
#include "graphics/resource_table.hpp"
#include "rendering/render_engine.hpp"

namespace
{
    // records frame and command flow without requiring a window or native graphics API
    class TestGraphicsDevice final : public Graphics::IGraphicsDevice
    {
    public:
        Graphics::FrameStatus beginStatus = Graphics::FrameStatus::Success;
        Graphics::FrameStatus endStatus = Graphics::FrameStatus::Success;
        uint32_t renderPassBegins = 0;
        uint32_t renderPassEnds = 0;

        bool Initialize(const Graphics::GraphicsDeviceDesc&) override { return true; }
        Graphics::GpuBufferHandle CreateBuffer(const Graphics::BufferDesc&) override { return {}; }
        Graphics::GpuTextureHandle CreateTexture(const Graphics::TextureDesc&) override { return {}; }
        Graphics::GpuSamplerHandle CreateSampler(const Graphics::SamplerDesc&) override { return {}; }
        Graphics::GpuShaderHandle CreateShader(const Graphics::ShaderProgramDesc&) override { return {}; }
        Graphics::GpuPipelineHandle CreateGraphicsPipeline(const Graphics::GraphicsPipelineDesc&) override { return {}; }
        void DestroyBuffer(Graphics::GpuBufferHandle) override {}
        void DestroyTexture(Graphics::GpuTextureHandle) override {}
        void DestroySampler(Graphics::GpuSamplerHandle) override {}
        void DestroyShader(Graphics::GpuShaderHandle) override {}
        void DestroyPipeline(Graphics::GpuPipelineHandle) override {}
        Graphics::FrameStatus BeginFrame() override { return beginStatus; }
        Graphics::FrameStatus EndFrame() override { return endStatus; }
        void OnResize(uint32_t, uint32_t) override {}
        void WaitIdle() override {}
        void Shutdown() override {}

        void BeginRenderPass(const Graphics::RenderPassDesc&) override { ++renderPassBegins; }
        void EndRenderPass() override { ++renderPassEnds; }
        void SetViewport(const Graphics::ViewportDesc&) override {}
        void SetPipeline(Graphics::GpuPipelineHandle) override {}
        void SetVertexBuffer(Graphics::GpuBufferHandle, const Graphics::VertexLayout&) override {}
        void SetIndexBuffer(Graphics::GpuBufferHandle) override {}
        void SetFrameConstants(const Graphics::FrameConstants&) override {}
        void SetMaterialResources(Graphics::GpuTextureHandle, Graphics::GpuSamplerHandle) override {}
        void SetDrawConstants(const Graphics::DrawConstants&) override {}
        void Draw(uint32_t) override {}
        void DrawIndexed(uint32_t) override {}
    };
}

namespace Tests
{
    bool RunGraphicsApiTests()
    {
        Graphics::ResourceTable<Graphics::GpuBufferHandle, int> first;
        Graphics::ResourceTable<Graphics::GpuBufferHandle, int> second;
        const Graphics::GpuBufferHandle original = first.Create(10);
        if (!original || !first.Get(original) || second.Get(original)) return false;
        if (!first.Destroy(original) || first.Get(original) || first.Destroy(original)) return false;
        const Graphics::GpuBufferHandle replacement = first.Create(20);
        if (!replacement || replacement == original || first.Get(original)) return false;
        first.Clear();
        const Graphics::GpuBufferHandle afterClear = first.Create(30);
        if (!afterClear || first.Get(replacement) || afterClear == replacement)
            return false;

        // verify that RenderEngine preserves recoverable and fatal device statuses
        auto device = std::make_unique<TestGraphicsDevice>();
        TestGraphicsDevice* deviceView = device.get();
        Assets::AssetManager assets;
        Rendering::RenderEngine renderEngine(std::move(device), assets);
        Graphics::Camera2D camera;
        camera.SetViewport(16.0f, 16.0f);

        deviceView->beginStatus = Graphics::FrameStatus::ResizeRequired;
        if (renderEngine.Render(camera, 16, 16) != Graphics::FrameStatus::ResizeRequired ||
            deviceView->renderPassBegins != 0) return false;

        deviceView->beginStatus = Graphics::FrameStatus::Success;
        deviceView->endStatus = Graphics::FrameStatus::DeviceLost;
        if (renderEngine.Render(camera, 16, 16) != Graphics::FrameStatus::Success ||
            renderEngine.EndFrame() != Graphics::FrameStatus::DeviceLost) return false;

        deviceView->endStatus = Graphics::FrameStatus::ResizeRequired;
        if (renderEngine.Render(camera, 16, 16) != Graphics::FrameStatus::Success ||
            renderEngine.EndFrame() != Graphics::FrameStatus::ResizeRequired) return false;

        return deviceView->renderPassBegins == 2 &&
            deviceView->renderPassEnds == 2;
    }
}
