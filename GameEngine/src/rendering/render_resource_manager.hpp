#pragma once

#include <cstdint>
#include <unordered_map>

#include "assets/asset_manager.hpp"
#include "graphics/graphics_device.hpp"

namespace Rendering
{
    // contains the GPU handles needed for one backend-neutral draw operation
    struct ResolvedDraw
    {
        Graphics::GpuBufferHandle vertexBuffer;
        Graphics::GpuBufferHandle indexBuffer;
        Graphics::GpuTextureHandle texture;
        Graphics::GpuSamplerHandle sampler;
        Graphics::GpuPipelineHandle pipeline;
        Graphics::VertexLayout layout;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    // uploads CPU assets on first use and caches their GPU representation
    class RenderResourceManager
    {
    public:
        RenderResourceManager(const Assets::AssetManager& assets, Graphics::IGraphicsDevice& device);
        bool Resolve(
            Assets::MeshHandle mesh,
            Assets::MaterialHandle material,
            ResolvedDraw& output);
        void Clear();

    private:
        // cache entries own only typed handles because the device owns native objects
        struct MeshGpu
        {
            Graphics::GpuBufferHandle vertexBuffer;
            Graphics::GpuBufferHandle indexBuffer;
            Graphics::VertexLayout layout;
            Graphics::PrimitiveTopology topology = Graphics::PrimitiveTopology::TRIANGLES;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
        };

        const Assets::AssetManager& _assets;
        Graphics::IGraphicsDevice& _device;
        std::unordered_map<Assets::AssetId, MeshGpu> _meshes;
        std::unordered_map<Assets::AssetId, Graphics::GpuTextureHandle> _textures;
        std::unordered_map<Assets::AssetId, Graphics::GpuShaderHandle> _shaders;
        std::unordered_map<uint64_t, Graphics::GpuPipelineHandle> _pipelines;
        Graphics::GpuSamplerHandle _sampler;

        // private resolvers keep asset lookup and upload outside the renderer loop
        MeshGpu* ResolveMesh(Assets::MeshHandle handle);
        Graphics::GpuTextureHandle ResolveTexture(Assets::TextureHandle handle);
        Graphics::GpuShaderHandle ResolveShader(Assets::ShaderHandle handle);
    };
}
