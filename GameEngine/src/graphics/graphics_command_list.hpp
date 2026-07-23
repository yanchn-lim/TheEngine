#pragma once

#include "graphics_descriptions.hpp"

namespace Graphics
{
    // records backend-neutral state and draw operations for the active frame
    class IGraphicsCommandList
    {
    public:
        virtual ~IGraphicsCommandList() = default;

        // pass and dynamic state must be set before draw operations
        virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void EndRenderPass() = 0;
        virtual void SetViewport(const ViewportDesc& viewport) = 0;
        virtual void SetScissor(const ScissorDesc& scissor) = 0;
        virtual void SetPipeline(GpuPipelineHandle pipeline) = 0;
        virtual void SetVertexBuffer(GpuBufferHandle buffer, const VertexLayout& layout) = 0;
        virtual void SetIndexBuffer(GpuBufferHandle buffer, IndexFormat format) = 0;
        // constants and material resources bind data used by the active pipeline
        virtual void SetFrameConstants(const FrameConstants& constants) = 0;
        virtual void SetMaterialResources(GpuTextureHandle texture, GpuSamplerHandle sampler) = 0;
        virtual void SetDrawConstants(const DrawConstants& constants) = 0;
        // draw operations consume the state set above
        virtual void Draw(uint32_t vertexCount) = 0;
        virtual void DrawIndexed(uint32_t indexCount) = 0;
        virtual void AddDebugMarker(const char* label) = 0;
    };
}
