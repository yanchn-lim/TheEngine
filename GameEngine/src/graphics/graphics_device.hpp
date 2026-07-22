#pragma once

#include "graphics_command_list.hpp"

namespace Graphics
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

    struct FrameContext
    {
        uint64_t frameNumber = 0;
    };

    // owns GPU resources, frame submission, and presentation for one backend
    class IGraphicsDevice
    {
    public:
        virtual ~IGraphicsDevice() = default;

        virtual bool Initialize(const GraphicsDeviceDesc& desc) = 0;
        virtual const GraphicsCapabilities& GetCapabilities() const = 0;

        // creation returns typed handles instead of native OpenGL or Vulkan objects
        virtual GpuBufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        virtual GpuTextureHandle CreateTexture(const TextureDesc& desc) = 0;
        virtual GpuSamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
        virtual GpuShaderHandle CreateShader(const ShaderProgramDesc& desc) = 0;
        virtual GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;

        virtual void DestroyBuffer(GpuBufferHandle handle) = 0;
        virtual void DestroyTexture(GpuTextureHandle handle) = 0;
        virtual void DestroySampler(GpuSamplerHandle handle) = 0;
        virtual void DestroyShader(GpuShaderHandle handle) = 0;
        virtual void DestroyPipeline(GpuPipelineHandle handle) = 0;

        // frame methods keep acquire, recording, submission, and presentation ordered
        virtual FrameStatus BeginFrame(FrameContext& context) = 0;
        virtual IGraphicsCommandList& GetCommandList(FrameContext& context) = 0;
        virtual FrameStatus EndFrame(FrameContext& context) = 0;
        virtual FrameStatus Present(FrameContext& context) = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void WaitIdle() = 0;
        virtual void Shutdown() = 0;
    };
}
