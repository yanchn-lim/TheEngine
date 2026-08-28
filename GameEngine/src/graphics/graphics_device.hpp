#pragma once

#include "graphics_descriptions.hpp"

namespace Ludus::Graphics
{
    // tells the renderer whether it can continue, skip, rebuild, or stop the frame
    enum class FrameStatus
    {
        Success,
        Skip,
        ResizeRequired,
        DeviceLost,
        Fatal
    };

    // owns gpu resources, frame recording, submission, and presentation.
    // one device supports one active frame and render pass at a time.
    class IGraphicsDevice
    {
    public:
        virtual ~IGraphicsDevice() = default;

        virtual bool Initialize(const GraphicsDeviceDesc& desc) = 0;

        // creation copies or uploads borrowed description data before it returns.
        // typed handles keep native opengl and vulkan objects behind this boundary.
        virtual GpuBufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        virtual GpuTextureHandle CreateTexture(const TextureDesc& desc) = 0;
        virtual GpuSamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
        virtual GpuShaderHandle CreateShader(const ShaderProgramDesc& desc) = 0;
        virtual GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
		virtual GpuRenderTargetHandle CreateRenderTarget(const RenderTargetDesc& desc) = 0;
		virtual GpuTextureHandle GetRenderTargetTexture(GpuRenderTargetHandle handle) const = 0;

        virtual void DestroyBuffer(GpuBufferHandle handle) = 0;
        virtual void DestroyTexture(GpuTextureHandle handle) = 0;
        virtual void DestroySampler(GpuSamplerHandle handle) = 0;
        virtual void DestroyShader(GpuShaderHandle handle) = 0;
        virtual void DestroyPipeline(GpuPipelineHandle handle) = 0;
		virtual void DestroyRenderTarget(GpuRenderTargetHandle handle) = 0;

        // frame methods keep acquire, recording, submission, and presentation ordered
        virtual FrameStatus BeginFrame() = 0;
        virtual FrameStatus EndFrame() = 0;

        // pass and draw methods operate on the active frame
        virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void EndRenderPass() = 0;
        virtual void SetViewport(const ViewportDesc& viewport) = 0;
        virtual void SetPipeline(GpuPipelineHandle pipeline) = 0;
        virtual void SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout) = 0;
        virtual void SetIndexBuffer(GpuBufferHandle buffer) = 0;
        virtual void SetFrameConstants(const FrameConstants& constants) = 0;
        virtual void SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler) = 0;
        virtual void SetDrawConstants(const DrawConstants& constants) = 0;
        virtual void Draw(uint32_t vertexCount) = 0;
        virtual void DrawIndexed(uint32_t indexCount) = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void WaitIdle() = 0;
        virtual void Shutdown() = 0;
    };
}
