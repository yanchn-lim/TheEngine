#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "assets/asset_manager.hpp"
#include "graphics/graphics_device.hpp"

namespace Ludus::Rendering
{
    // contains the GPU handles needed for one backend-neutral draw operation
    struct ResolvedDraw
    {
        Ludus::Graphics::GpuBufferHandle vertexBuffer;
        Ludus::Graphics::GpuBufferHandle indexBuffer;
        Ludus::Graphics::GpuTextureHandle texture;
        Ludus::Graphics::GpuSamplerHandle sampler;
        Ludus::Graphics::GpuPipelineHandle pipeline;
        Ludus::Graphics::VertexLayout layout;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    // uploads CPU assets on first use and caches their GPU representation
    class RenderResourceManager
    {
    public:
        RenderResourceManager(const Ludus::Assets::AssetManager& assets, Ludus::Graphics::IGraphicsDevice& device);
        bool Resolve(
            Ludus::Assets::MeshHandle mesh,
            Ludus::Assets::MaterialHandle materialOverride,
            std::vector<ResolvedDraw>& output);
        void Clear();

    private:
        // cache entries own only typed handles because the device owns native objects
        struct SurfaceGpu
        {
            Ludus::Graphics::GpuBufferHandle vertexBuffer;
            Ludus::Graphics::GpuBufferHandle indexBuffer;
            Ludus::Graphics::VertexLayout layout;
            Ludus::Graphics::PrimitiveTopology topology = Ludus::Graphics::PrimitiveTopology::TRIANGLES;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
            std::unordered_map<Ludus::Assets::AssetId, Ludus::Graphics::GpuPipelineHandle> pipelines;
        };

        struct MeshGpu
        {
            std::vector<SurfaceGpu> surfaces;
        };

        const Ludus::Assets::AssetManager& _assets;
        Ludus::Graphics::IGraphicsDevice& _device;
        std::unordered_map<Ludus::Assets::AssetId, MeshGpu> _meshes;
        std::unordered_map<Ludus::Assets::AssetId, Ludus::Graphics::GpuTextureHandle> _textures;
        std::unordered_map<Ludus::Assets::AssetId, Ludus::Graphics::GpuShaderHandle> _shaders;
        Ludus::Graphics::GpuSamplerHandle _sampler;

        // private resolvers keep asset lookup and upload outside the renderer loop
        MeshGpu* ResolveMesh(Ludus::Assets::MeshHandle handle);
        Ludus::Graphics::GpuTextureHandle ResolveTexture(Ludus::Assets::TextureHandle handle);
        Ludus::Graphics::GpuShaderHandle ResolveShader(Ludus::Assets::ShaderHandle handle);
    };
}
