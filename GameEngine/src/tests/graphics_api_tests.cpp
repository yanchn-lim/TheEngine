#include "graphics_api_tests.hpp"

#include <vector>

#include "assets/asset_manager.hpp"
#include "graphics/graphics_command_list.hpp"
#include "graphics/graphics_device.hpp"
#include "graphics/graphics_handles.hpp"
#include "graphics/resource_table.hpp"
#include "rendering/render_world.hpp"
#include "rendering/renderer.hpp"

namespace
{
    // records frame and command flow without requiring a window or native graphics API
    class TestGraphicsDevice final : public Graphics::IGraphicsDevice, public Graphics::IGraphicsCommandList
    {
    public:
        Graphics::FrameStatus beginStatus = Graphics::FrameStatus::Success;
        Graphics::FrameStatus endStatus = Graphics::FrameStatus::Success;
        Graphics::FrameStatus presentStatus = Graphics::FrameStatus::Success;
        uint32_t renderPassBegins = 0;
        uint32_t renderPassEnds = 0;
        uint32_t waits = 0;

        bool Initialize(const Graphics::GraphicsDeviceDesc&) override { return true; }
        const Graphics::GraphicsCapabilities& GetCapabilities() const override { return capabilities; }
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
        Graphics::FrameStatus BeginFrame(Graphics::FrameContext&) override { return beginStatus; }
        Graphics::IGraphicsCommandList& GetCommandList(Graphics::FrameContext&) override { return *this; }
        Graphics::FrameStatus EndFrame(Graphics::FrameContext&) override { return endStatus; }
        Graphics::FrameStatus Present(Graphics::FrameContext&) override { return presentStatus; }
        void OnResize(uint32_t, uint32_t) override {}
        void WaitIdle() override { ++waits; }
        void Shutdown() override {}

        void BeginRenderPass(const Graphics::RenderPassDesc&) override { ++renderPassBegins; }
        void EndRenderPass() override { ++renderPassEnds; }
        void SetViewport(const Graphics::ViewportDesc&) override {}
        void SetScissor(const Graphics::ScissorDesc&) override {}
        void SetPipeline(Graphics::GpuPipelineHandle) override {}
        void SetVertexBuffer(Graphics::GpuBufferHandle, const Graphics::VertexLayout&) override {}
        void SetIndexBuffer(Graphics::GpuBufferHandle, Graphics::IndexFormat) override {}
        void SetFrameConstants(const Graphics::FrameConstants&) override {}
        void SetMaterialResources(Graphics::GpuTextureHandle, Graphics::GpuSamplerHandle) override {}
        void SetDrawConstants(const Graphics::DrawConstants&) override {}
        void Draw(uint32_t) override {}
        void DrawIndexed(uint32_t) override {}
        void AddDebugMarker(const char*) override {}

    private:
        Graphics::GraphicsCapabilities capabilities{ "test", false };
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

        Rendering::RenderWorld world;
        Rendering::MeshInstanceDesc mesh;
        mesh.mesh = Assets::MeshHandle{ 1 };
        const Rendering::RenderInstanceHandle persistent = world.CreateMeshInstance(mesh);
        world.DrawMeshOnce(mesh);
        std::vector<Rendering::RenderItem> items;
        world.Collect(Rendering::DefaultRenderLayer, items);
        if (items.size() != 2 || !world.Destroy(persistent) || world.SetVisible(persistent, true)) return false;
        world.EndFrame();
        world.Collect(Rendering::DefaultRenderLayer, items);
        if (!items.empty()) return false;

        // verify that Renderer preserves recoverable and fatal device statuses
        TestGraphicsDevice device;
        Assets::AssetManager assets;
        Rendering::Renderer renderer(device, assets, world);
        Graphics::Camera2D camera;
        camera.SetViewport(16.0f, 16.0f);

        device.beginStatus = Graphics::FrameStatus::ResizeRequired;
        if (renderer.Render(camera, 16, 16) != Graphics::FrameStatus::ResizeRequired ||
            device.renderPassBegins != 0) return false;

        device.beginStatus = Graphics::FrameStatus::Success;
        device.endStatus = Graphics::FrameStatus::DeviceLost;
        if (renderer.Render(camera, 16, 16) != Graphics::FrameStatus::Success ||
            renderer.EndFrame() != Graphics::FrameStatus::DeviceLost ||
            renderer.Present() != Graphics::FrameStatus::Skip) return false;

        device.endStatus = Graphics::FrameStatus::Success;
        device.presentStatus = Graphics::FrameStatus::ResizeRequired;
        if (renderer.Render(camera, 16, 16) != Graphics::FrameStatus::Success ||
            renderer.EndFrame() != Graphics::FrameStatus::Success ||
            renderer.Present() != Graphics::FrameStatus::ResizeRequired) return false;

        const uint32_t waitsBeforeInvalidation = device.waits;
        renderer.Invalidate(Assets::MeshHandle{ 99 });
        return device.waits == waitsBeforeInvalidation + 1 &&
            device.renderPassBegins == 2 && device.renderPassEnds == 2;
    }
}
