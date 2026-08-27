#include "render_resource_manager.hpp"

#include "debug/debug.hpp"

namespace Ludus::Rendering
{
    RenderResourceManager::RenderResourceManager(const Ludus::Assets::AssetManager& assets, Ludus::Graphics::IGraphicsDevice& device)
        : _assets(assets), _device(device)
    {
    }

    bool RenderResourceManager::Resolve(
        Ludus::Assets::MeshHandle meshHandle,
        Ludus::Assets::MaterialHandle materialOverride,
        std::vector<ResolvedDraw>& output)
    {
        MeshGpu* mesh = ResolveMesh(meshHandle);
        const Ludus::Assets::MeshAsset* asset = _assets.Get(meshHandle);

        if (!mesh || !asset || mesh->surfaces.size() != asset->surfaces.size())
            return false;

        if (!_sampler)
            _sampler = _device.CreateSampler({ true, true });

        if (!_sampler)
            return false;

        output.clear();
        output.reserve(mesh->surfaces.size());

        for (size_t index = 0; index < mesh->surfaces.size(); ++index)
        {
            SurfaceGpu& surface = mesh->surfaces[index];
            const Ludus::Assets::MeshSurface& source = asset->surfaces[index];
            const Ludus::Assets::MaterialHandle materialHandle =
                materialOverride ? materialOverride : source.material;
            const Ludus::Assets::MaterialAsset* material = _assets.Get(materialHandle);

            if (!material)
            {
                output.clear();
                return false;
            }

            const Ludus::Graphics::GpuShaderHandle shader = ResolveShader(material->shader);
            const Ludus::Graphics::GpuTextureHandle texture = ResolveTexture(material->texture);

            if (!shader || !texture)
            {
                output.clear();
                return false;
            }

            Ludus::Graphics::GpuPipelineHandle pipeline;
            const auto existing = surface.pipelines.find(materialHandle.id);
            if (existing != surface.pipelines.end())
                pipeline = existing->second;
            else
            {
                Ludus::Graphics::GraphicsPipelineDesc desc;
                desc.shader = shader;
                desc.vertexLayout = surface.layout;
                desc.topology = surface.topology;
                desc.renderState = material->state;
                pipeline = _device.CreateGraphicsPipeline(desc);

                if (!pipeline)
                {
                    output.clear();
                    return false;
                }

                surface.pipelines.emplace(materialHandle.id, pipeline);
            }

            ResolvedDraw draw;
            draw.vertexBuffer = surface.vertexBuffer;
            draw.indexBuffer = surface.indexBuffer;
            draw.texture = texture;
            draw.sampler = _sampler;
            draw.pipeline = pipeline;
            draw.layout = surface.layout;
            draw.vertexCount = surface.vertexCount;
            draw.indexCount = surface.indexCount;
            output.push_back(draw);
        }

        return true;
    }

    RenderResourceManager::MeshGpu* RenderResourceManager::ResolveMesh(Ludus::Assets::MeshHandle handle)
    {
        // upload vertex and index data only when the asset first becomes visible
        if (const auto existing = _meshes.find(handle.id); existing != _meshes.end())
            return &existing->second;

        const Ludus::Assets::MeshAsset* asset = _assets.Get(handle);

        if (!asset || asset->surfaces.empty())
            return nullptr;

        MeshGpu mesh;
        mesh.surfaces.reserve(asset->surfaces.size());

        for (const Ludus::Assets::MeshSurface& source : asset->surfaces)
        {
            SurfaceGpu surface;
            surface.layout = Ludus::Assets::CreateMeshVertexLayout();
            surface.topology = source.topology;
            surface.vertexCount = static_cast<uint32_t>(source.vertices.size());
            surface.indexCount = static_cast<uint32_t>(source.indices.size());
            surface.vertexBuffer = _device.CreateBuffer({ source.vertices.data(),
                source.vertices.size() * sizeof(Ludus::Assets::MeshVertex), Ludus::Graphics::BufferUsage::Vertex });

            if (!source.indices.empty())
                surface.indexBuffer = _device.CreateBuffer({ source.indices.data(),
                    source.indices.size() * sizeof(uint32_t), Ludus::Graphics::BufferUsage::Index });

            if (!surface.vertexBuffer || (surface.indexCount && !surface.indexBuffer))
            {
                _device.DestroyBuffer(surface.indexBuffer);
                _device.DestroyBuffer(surface.vertexBuffer);

                for (SurfaceGpu& created : mesh.surfaces)
                {
                    _device.DestroyBuffer(created.indexBuffer);
                    _device.DestroyBuffer(created.vertexBuffer);
                }

                return nullptr;
            }

            mesh.surfaces.push_back(std::move(surface));
        }

        return &_meshes.emplace(handle.id, std::move(mesh)).first->second;
    }

    Ludus::Graphics::GpuTextureHandle RenderResourceManager::ResolveTexture(Ludus::Assets::TextureHandle handle)
    {
        // upload texture pixels only when the asset first becomes visible
        if (const auto existing = _textures.find(handle.id); existing != _textures.end())
            return existing->second;

        const Ludus::Assets::TextureAsset* asset = _assets.Get(handle);

        if (!asset || asset->pixels.empty())
            return {};

        const Ludus::Graphics::GpuTextureHandle texture =
            _device.CreateTexture({ asset->pixels.data(), asset->width, asset->height });

        if (texture)
            _textures.emplace(handle.id, texture);

        return texture;
    }

    Ludus::Graphics::GpuShaderHandle RenderResourceManager::ResolveShader(Ludus::Assets::ShaderHandle handle)
    {
        if (const auto existing = _shaders.find(handle.id); existing != _shaders.end())
            return existing->second;

        const Ludus::Assets::ShaderAsset* asset = _assets.Get(handle);

        if (!asset)
            return {};

        Ludus::Graphics::ShaderProgramDesc desc;
        desc.vertexSource = asset->vertexSource;
        desc.fragmentSource = asset->fragmentSource;
        desc.vertexSpirv = asset->vertexSpirv;
        desc.fragmentSpirv = asset->fragmentSpirv;
        desc.label = asset->label;

        const Ludus::Graphics::GpuShaderHandle shader = _device.CreateShader(desc);
        if (shader)
            _shaders.emplace(handle.id, shader);

        return shader;
    }

    void RenderResourceManager::Clear()
    {
        // destroy dependent pipelines before the resources they reference
        _device.WaitIdle();
        for (auto& [meshId, mesh] : _meshes)
            for (SurfaceGpu& surface : mesh.surfaces)
                for (auto& [materialId, pipeline] : surface.pipelines)
                    _device.DestroyPipeline(pipeline);

        for (auto& [key, shader] : _shaders)
            _device.DestroyShader(shader);

        _device.DestroySampler(_sampler);
        _sampler = {};

        for (auto& [key, texture] : _textures)
            _device.DestroyTexture(texture);

        for (auto& [key, mesh] : _meshes)
        {
            for (SurfaceGpu& surface : mesh.surfaces)
            {
                _device.DestroyBuffer(surface.indexBuffer);
                _device.DestroyBuffer(surface.vertexBuffer);
            }
        }

        _shaders.clear();
        _textures.clear();
        _meshes.clear();
    }
}
