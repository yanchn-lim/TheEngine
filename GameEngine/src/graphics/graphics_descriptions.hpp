#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "graphics_handles.hpp"
#include "primitive_topology.hpp"
#include "render_state.hpp"
#include "vertex_layout.hpp"

namespace Graphics
{
    // backend-neutral enums describe intent without exposing OpenGL or Vulkan values
    enum class BufferUsage { Vertex, Index };

    struct GraphicsDeviceDesc
    {
        void* window = nullptr;
        bool vsync = true;
    };

    // resource descriptions carry CPU source data and a diagnostic label into the device
    struct BufferDesc
    {
        const void* data = nullptr;
        size_t size = 0;
        BufferUsage usage = BufferUsage::Vertex;
    };

    struct TextureDesc
    {
        const unsigned char* pixels = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct SamplerDesc
    {
        bool linear = true;
        bool repeat = true;
    };

    struct ShaderProgramDesc
    {
        std::string vertexSource;
        std::string fragmentSource;
        std::vector<uint32_t> vertexSpirv;
        std::vector<uint32_t> fragmentSpirv;
        std::string label;
    };

    struct GraphicsPipelineDesc
    {
        GpuShaderHandle shader;
        VertexLayout vertexLayout;
        PrimitiveTopology topology = PrimitiveTopology::TRIANGLES;
        RenderState renderState{};
    };

    // frame constants are shared by all draws in one camera view
    struct FrameConstants
    {
        glm::mat4 view{ 1.0f };
        glm::mat4 projection{ 1.0f };
    };

    // draw constants change for each render item
    struct DrawConstants
    {
        glm::mat4 model{ 1.0f };
        glm::vec4 tint{ 1.0f };
        glm::vec4 uvRect{ 0.0f, 0.0f, 1.0f, 1.0f };
    };

    // render-pass data controls attachment clearing for the current view
    struct RenderPassDesc
    {
        glm::vec4 clearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
        bool clearColorTarget = true;
        bool clearDepthTarget = true;
    };

    struct ViewportDesc
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

}
