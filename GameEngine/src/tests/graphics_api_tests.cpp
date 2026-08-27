#include "graphics_api_tests.hpp"

#include "assets/asset_manager.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/graphics_handles.hpp"
#include "graphics/resource_table.hpp"
#include "rendering/render_engine.hpp"

namespace
{
    // records frame and command flow without requiring a window or native graphics API
    class TestGraphicsDevice final : public Ludus::Graphics::IGraphicsDevice
    {
    public:
        Ludus::Graphics::FrameStatus beginStatus = Ludus::Graphics::FrameStatus::Success;
        Ludus::Graphics::FrameStatus endStatus = Ludus::Graphics::FrameStatus::Success;
        uint32_t renderPassBegins = 0;
        uint32_t renderPassEnds = 0;

        bool Initialize(const Ludus::Graphics::GraphicsDeviceDesc&) override { return true; }
        Ludus::Graphics::GpuBufferHandle CreateBuffer(const Ludus::Graphics::BufferDesc&) override { return {}; }
        Ludus::Graphics::GpuTextureHandle CreateTexture(const Ludus::Graphics::TextureDesc&) override { return {}; }
        Ludus::Graphics::GpuSamplerHandle CreateSampler(const Ludus::Graphics::SamplerDesc&) override { return {}; }
        Ludus::Graphics::GpuShaderHandle CreateShader(const Ludus::Graphics::ShaderProgramDesc&) override { return {}; }
        Ludus::Graphics::GpuPipelineHandle CreateGraphicsPipeline(const Ludus::Graphics::GraphicsPipelineDesc&) override { return {}; }
        void DestroyBuffer(Ludus::Graphics::GpuBufferHandle) override {}
        void DestroyTexture(Ludus::Graphics::GpuTextureHandle) override {}
        void DestroySampler(Ludus::Graphics::GpuSamplerHandle) override {}
        void DestroyShader(Ludus::Graphics::GpuShaderHandle) override {}
        void DestroyPipeline(Ludus::Graphics::GpuPipelineHandle) override {}
        Ludus::Graphics::FrameStatus BeginFrame() override { return beginStatus; }
        Ludus::Graphics::FrameStatus EndFrame() override { return endStatus; }
        void OnResize(uint32_t, uint32_t) override {}
        void WaitIdle() override {}
        void Shutdown() override {}

        void BeginRenderPass(const Ludus::Graphics::RenderPassDesc&) override { ++renderPassBegins; }
        void EndRenderPass() override { ++renderPassEnds; }
        void SetViewport(const Ludus::Graphics::ViewportDesc&) override {}
        void SetPipeline(Ludus::Graphics::GpuPipelineHandle) override {}
        void SetVertexBuffer(Ludus::Graphics::GpuBufferHandle, const Ludus::Graphics::VertexLayout&) override {}
        void SetIndexBuffer(Ludus::Graphics::GpuBufferHandle) override {}
        void SetFrameConstants(const Ludus::Graphics::FrameConstants&) override {}
        void SetMaterialResources(Ludus::Graphics::GpuTextureHandle, Ludus::Graphics::GpuSamplerHandle) override {}
        void SetDrawConstants(const Ludus::Graphics::DrawConstants&) override {}
        void Draw(uint32_t) override {}
        void DrawIndexed(uint32_t) override {}
    };
}

namespace Tests
{
    bool RunGraphicsApiTests()
    {
        Ludus::Graphics::ResourceTable<Ludus::Graphics::GpuBufferHandle, int> first;
        Ludus::Graphics::ResourceTable<Ludus::Graphics::GpuBufferHandle, int> second;
        Ludus::Graphics::ResourceTable<Ludus::Graphics::GpuBufferHandle, float> differentResourceType;
        const Ludus::Graphics::GpuBufferHandle original = first.Create(10);
        if (!original || !first.Get(original) ||
            second.Get(original) || differentResourceType.Get(original))
            return false;
        if (!first.Destroy(original) || first.Get(original) || first.Destroy(original)) return false;
        const Ludus::Graphics::GpuBufferHandle replacement = first.Create(20);
        if (!replacement || replacement == original || first.Get(original)) return false;
        first.Clear();
        const Ludus::Graphics::GpuBufferHandle afterClear = first.Create(30);
        if (!afterClear || first.Get(replacement) || afterClear == replacement)
            return false;

        // verify that RenderEngine preserves recoverable and fatal device statuses
        auto device = std::make_unique<TestGraphicsDevice>();
        TestGraphicsDevice* deviceView = device.get();
        Ludus::Assets::AssetManager assets;
        Ludus::Rendering::RenderEngine renderEngine(std::move(device), assets);
        Ludus::Graphics::Camera2D camera;
        camera.SetViewport(16.0f, 16.0f);

        deviceView->beginStatus = Ludus::Graphics::FrameStatus::ResizeRequired;
        if (renderEngine.Render(camera, 16, 16) != Ludus::Graphics::FrameStatus::ResizeRequired ||
            deviceView->renderPassBegins != 0) return false;

        deviceView->beginStatus = Ludus::Graphics::FrameStatus::Success;
        deviceView->endStatus = Ludus::Graphics::FrameStatus::DeviceLost;
        if (renderEngine.Render(camera, 16, 16) != Ludus::Graphics::FrameStatus::Success ||
            renderEngine.EndFrame() != Ludus::Graphics::FrameStatus::DeviceLost) return false;

        deviceView->endStatus = Ludus::Graphics::FrameStatus::ResizeRequired;
        if (renderEngine.Render(camera, 16, 16) != Ludus::Graphics::FrameStatus::Success ||
            renderEngine.EndFrame() != Ludus::Graphics::FrameStatus::ResizeRequired) return false;

        return deviceView->renderPassBegins == 2 &&
            deviceView->renderPassEnds == 2;
    }
}
