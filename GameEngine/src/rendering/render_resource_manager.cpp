#include "render_resource_manager.hpp"

#include "debug/debug.hpp"

namespace Rendering
{
    RenderResourceManager::RenderResourceManager(const Assets::AssetManager& assets, Graphics::IGraphicsDevice& device)
        : _assets(assets), _device(device)
    {
    }

    bool RenderResourceManager::Resolve(Assets::MeshHandle meshHandle, Assets::MaterialHandle materialHandle,
        ResolvedDraw& output, Assets::TextureHandle textureOverride)
    {
        // resolve logical assets before assembling the final draw data
        MeshGpu* mesh = ResolveMesh(meshHandle);
        MaterialGpu* material = ResolveMaterial(materialHandle);
        if (!mesh || !material)
            return false;

        const uint64_t key = (static_cast<uint64_t>(meshHandle.id) << 32) | materialHandle.id;
        // one pipeline is cached for each current mesh and material pair
        Graphics::GpuPipelineHandle pipeline;
        if (const auto existing = _pipelines.find(key); existing != _pipelines.end())
            pipeline = existing->second;
        else
        {
            Graphics::GraphicsPipelineDesc desc;
            desc.shader = material->shader;
            desc.vertexLayout = mesh->layout;
            desc.topology = mesh->topology;
            desc.renderState = material->state;
            desc.label = "Material pipeline";
            pipeline = _device.CreateGraphicsPipeline(desc);
            if (!pipeline)
                return false;
            _pipelines.emplace(key, pipeline);
        }

        output.vertexBuffer = mesh->vertexBuffer;
        output.indexBuffer = mesh->indexBuffer;
        TextureGpu* selectedTexture = textureOverride ? ResolveTexture(textureOverride) : &material->texture;
        if (!selectedTexture) return false;
        output.texture = selectedTexture->texture;
        output.sampler = selectedTexture->sampler;
        output.pipeline = pipeline;
        output.layout = mesh->layout;
        output.vertexCount = mesh->vertexCount;
        output.indexCount = mesh->indexCount;
        return true;
    }

    RenderResourceManager::MeshGpu* RenderResourceManager::ResolveMesh(Assets::MeshHandle handle)
    {
        // upload vertex and index data only when the asset first becomes visible
        if (const auto existing = _meshes.find(handle.id); existing != _meshes.end()) return &existing->second;
        const Assets::MeshAsset* asset = _assets.Get(handle);
        if (!asset || asset->data.vertices.empty()) return nullptr;

        MeshGpu gpu;
        gpu.layout = Assets::CreateMeshVertexLayout();
        gpu.topology = asset->data.topology;
        gpu.vertexCount = static_cast<uint32_t>(asset->data.vertices.size());
        gpu.indexCount = static_cast<uint32_t>(asset->data.indices.size());
        gpu.vertexBuffer = _device.CreateBuffer({ asset->data.vertices.data(),
            asset->data.vertices.size() * sizeof(Assets::MeshVertex), Graphics::BufferUsage::Vertex, asset->label + " vertices" });
        if (!asset->data.indices.empty())
            gpu.indexBuffer = _device.CreateBuffer({ asset->data.indices.data(),
                asset->data.indices.size() * sizeof(uint32_t), Graphics::BufferUsage::Index, asset->label + " indices" });
        if (!gpu.vertexBuffer || (gpu.indexCount && !gpu.indexBuffer))
        {
            _device.DestroyBuffer(gpu.indexBuffer);
            _device.DestroyBuffer(gpu.vertexBuffer);
            return nullptr;
        }
        return &_meshes.emplace(handle.id, std::move(gpu)).first->second;
    }

    RenderResourceManager::TextureGpu* RenderResourceManager::ResolveTexture(Assets::TextureHandle handle)
    {
        // texture pixels and sampler settings become separate GPU resources
        if (const auto existing = _textures.find(handle.id); existing != _textures.end()) return &existing->second;
        const Assets::TextureAsset* asset = _assets.Get(handle);
        if (!asset || asset->pixels.empty()) return nullptr;
        TextureGpu gpu;
        gpu.texture = _device.CreateTexture({ asset->pixels.data(), asset->width, asset->height,
            Graphics::TextureFormat::RGBA8, asset->label });
        gpu.sampler = _device.CreateSampler({ true, true, asset->label + " sampler" });
        if (!gpu.texture || !gpu.sampler)
        {
            _device.DestroySampler(gpu.sampler);
            _device.DestroyTexture(gpu.texture);
            return nullptr;
        }
        return &_textures.emplace(handle.id, std::move(gpu)).first->second;
    }

    Graphics::GpuShaderHandle RenderResourceManager::ResolveShader(Assets::ShaderHandle handle)
    {
        if (const auto existing = _shaders.find(handle.id); existing != _shaders.end()) return existing->second;
        const Assets::ShaderAsset* asset = _assets.Get(handle);
        if (!asset) return {};
        Graphics::ShaderProgramDesc desc;
        desc.vertexSource = asset->vertexSource;
        desc.fragmentSource = asset->fragmentSource;
        desc.vertexSpirv = asset->vertexSpirv;
        desc.fragmentSpirv = asset->fragmentSpirv;
        desc.label = asset->label;
        const Graphics::GpuShaderHandle shader = _device.CreateShader(desc);
        if (shader) _shaders.emplace(handle.id, shader);
        return shader;
    }

    RenderResourceManager::MaterialGpu* RenderResourceManager::ResolveMaterial(Assets::MaterialHandle handle)
    {
        if (const auto existing = _materials.find(handle.id); existing != _materials.end()) return &existing->second;
        const Assets::MaterialAsset* asset = _assets.Get(handle);
        if (!asset) return nullptr;
        MaterialGpu gpu;
        gpu.shaderAsset = asset->shader;
        gpu.textureAsset = asset->texture;
        gpu.shader = ResolveShader(asset->shader);
        TextureGpu* texture = ResolveTexture(asset->texture);
        if (!gpu.shader || !texture) return nullptr;
        gpu.texture = *texture;
        gpu.state = asset->state;
        return &_materials.emplace(handle.id, std::move(gpu)).first->second;
    }

    void RenderResourceManager::Invalidate(Assets::MeshHandle handle)
    {
        if (!handle) return;
        _device.WaitIdle();
        DestroyPipelinesForMesh(handle.id);
        if (const auto found = _meshes.find(handle.id); found != _meshes.end())
        {
            _device.DestroyBuffer(found->second.indexBuffer);
            _device.DestroyBuffer(found->second.vertexBuffer);
            _meshes.erase(found);
        }
    }

    void RenderResourceManager::Invalidate(Assets::TextureHandle handle)
    {
        if (!handle) return;
        _device.WaitIdle();
        for (auto material = _materials.begin(); material != _materials.end();)
        {
            if (material->second.textureAsset.id == handle.id) material = _materials.erase(material);
            else ++material;
        }
        if (const auto found = _textures.find(handle.id); found != _textures.end())
        {
            _device.DestroySampler(found->second.sampler);
            _device.DestroyTexture(found->second.texture);
            _textures.erase(found);
        }
    }

    void RenderResourceManager::Invalidate(Assets::ShaderHandle handle)
    {
        if (!handle) return;
        _device.WaitIdle();
        for (auto material = _materials.begin(); material != _materials.end();)
        {
            if (material->second.shaderAsset.id == handle.id)
            {
                DestroyPipelinesForMaterial(material->first);
                material = _materials.erase(material);
            }
            else ++material;
        }
        if (const auto found = _shaders.find(handle.id); found != _shaders.end())
        {
            _device.DestroyShader(found->second);
            _shaders.erase(found);
        }
    }

    void RenderResourceManager::Invalidate(Assets::MaterialHandle handle)
    {
        if (!handle) return;
        _device.WaitIdle();
        DestroyPipelinesForMaterial(handle.id);
        _materials.erase(handle.id);
    }

    void RenderResourceManager::DestroyPipelinesForMesh(Assets::AssetId id)
    {
        for (auto pipeline = _pipelines.begin(); pipeline != _pipelines.end();)
        {
            if (static_cast<Assets::AssetId>(pipeline->first >> 32) == id)
            {
                _device.DestroyPipeline(pipeline->second);
                pipeline = _pipelines.erase(pipeline);
            }
            else ++pipeline;
        }
    }

    void RenderResourceManager::DestroyPipelinesForMaterial(Assets::AssetId id)
    {
        for (auto pipeline = _pipelines.begin(); pipeline != _pipelines.end();)
        {
            if (static_cast<Assets::AssetId>(pipeline->first) == id)
            {
                _device.DestroyPipeline(pipeline->second);
                pipeline = _pipelines.erase(pipeline);
            }
            else ++pipeline;
        }
    }

    void RenderResourceManager::Clear()
    {
        // destroy dependent pipelines before the resources they reference
        _device.WaitIdle();
        for (auto& [key, pipeline] : _pipelines) _device.DestroyPipeline(pipeline);
        for (auto& [key, shader] : _shaders) _device.DestroyShader(shader);
        for (auto& [key, texture] : _textures)
        {
            _device.DestroySampler(texture.sampler);
            _device.DestroyTexture(texture.texture);
        }
        for (auto& [key, mesh] : _meshes)
        {
            _device.DestroyBuffer(mesh.indexBuffer);
            _device.DestroyBuffer(mesh.vertexBuffer);
        }
        _pipelines.clear();
        _materials.clear();
        _shaders.clear();
        _textures.clear();
        _meshes.clear();
    }
}
