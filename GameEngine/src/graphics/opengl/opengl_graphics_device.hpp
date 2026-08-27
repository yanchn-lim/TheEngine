#pragma once

#include <cstdint>

#include "graphics/graphics_device.hpp"
#include "graphics/resource_table.hpp"

struct GLFWwindow;

namespace Ludus::Graphics
{
    // implements IGraphicsDevice with immediate OpenGL state changes and draws
    class OpenGLGraphicsDevice final : public IGraphicsDevice
    {
    public:
        bool Initialize(const GraphicsDeviceDesc& desc) override;

        GpuBufferHandle CreateBuffer(const BufferDesc& desc) override;
        GpuTextureHandle CreateTexture(const TextureDesc& desc) override;
        GpuSamplerHandle CreateSampler(const SamplerDesc& desc) override;
        GpuShaderHandle CreateShader(const ShaderProgramDesc& desc) override;
        GpuPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
		GpuRenderTargetHandle CreateRenderTarget(const RenderTargetDesc& desc) override;
		GpuTextureHandle GetRenderTargetTexture(GpuRenderTargetHandle handle) const override;

        void DestroyBuffer(GpuBufferHandle handle) override;
        void DestroyTexture(GpuTextureHandle handle) override;
        void DestroySampler(GpuSamplerHandle handle) override;
        void DestroyShader(GpuShaderHandle handle) override;
        void DestroyPipeline(GpuPipelineHandle handle) override;
		void DestroyRenderTarget(GpuRenderTargetHandle handle) override;

        FrameStatus BeginFrame() override;
        FrameStatus EndFrame() override;
        void OnResize(uint32_t width, uint32_t height) override;
        void WaitIdle() override;
        void Shutdown() override;

        void BeginRenderPass(const RenderPassDesc& desc) override;
        void EndRenderPass() override;
        void SetViewport(const ViewportDesc& viewport) override;
        void SetPipeline(GpuPipelineHandle pipeline) override;
        void SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout) override;
        void SetIndexBuffer(GpuBufferHandle buffer) override;
        void SetFrameConstants(const FrameConstants& constants) override;
        void SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler) override;
        void SetDrawConstants(const DrawConstants& constants) override;
        void Draw(uint32_t vertexCount) override;
        void DrawIndexed(uint32_t indexCount) override;

		uint32_t NativeTexture(GpuTextureHandle handle) const;

    private:
        // native OpenGL names stay behind typed resource tables
        struct BufferResource { uint32_t id = 0; };
        struct TextureResource { uint32_t id = 0; };
        struct SamplerResource { uint32_t id = 0; };
        struct ShaderResource { uint32_t program = 0; };
        struct PipelineResource { GraphicsPipelineDesc desc; };
		struct RenderTargetResource
		{
			uint32_t framebuffer = 0;
			uint32_t depth = 0;
			GpuTextureHandle color;
		};

        GLFWwindow* _window = nullptr;
        ResourceTable<GpuBufferHandle, BufferResource> _buffers;
        ResourceTable<GpuTextureHandle, TextureResource> _textures;
        ResourceTable<GpuSamplerHandle, SamplerResource> _samplers;
        ResourceTable<GpuShaderHandle, ShaderResource> _shaders;
        ResourceTable<GpuPipelineHandle, PipelineResource> _pipelines;
		ResourceTable<GpuRenderTargetHandle, RenderTargetResource> _renderTargets;
        GpuPipelineHandle _activePipeline;
        uint32_t _vertexArray = 0;

        // helpers translate shared pipeline state into OpenGL state
        uint32_t ActiveProgram() const;
        void ApplyRenderState(const RenderState& state);
    };
}
