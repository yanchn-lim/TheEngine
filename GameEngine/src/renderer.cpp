#include <glad/glad.h>

#include "debug.hpp"
#include "profiler.hpp"

#include "renderer.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "vertex.hpp"

namespace Graphics
{
    void InstanceBatch::Init(uint meshVao, uint meshVbo, uint meshIbo)
    {
        // Create our own VAO so we can add instance attrib pointers on top of
        // the geometry layout already set up in the mesh VAO.
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &instanceVbo);

        glBindVertexArray(vao);

        // --- Re-bind geometry data from the mesh into this VAO ---
        // We point at the same VBO/IBO but rebind them here because VAO state
        // is per-VAO; the mesh VAO has them, this one does not yet.
        glBindBuffer(GL_ARRAY_BUFFER, meshVbo);

        // slot 0 = aPos (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(0);

        // slot 1 = aCol (vec3)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));
        glEnableVertexAttribArray(1);

        // slot 2 = aUV (vec2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
        glEnableVertexAttribArray(2);

        // IBO must be bound inside the VAO so glDrawElementsInstanced works.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIbo);

        // --- Instance VBO layout ---
        glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

        // slot 3 = iPosition (vec3) + iRotation (float) packed as vec4
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, position));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1); // advance once per instance, not per vertex

        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, rotation));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        // slot 5 = iScale (vec3) + pad (float) packed as vec4
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, scale));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        // slot 6 = iTint (vec4)
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, tint));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    void InstanceBatch::Upload()
    {
        if (instances.empty()) return;

        glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);

        uint needed = (uint)instances.size();

        //if (needed > instanceCap)
        //{
        //    // Allocate more space with GL_DYNAMIC_DRAW since this buffer is
        //    // updated every frame. Over-allocate to avoid realloc every frame.
        //    instanceCap = needed * 2;
        //    glBufferData(GL_ARRAY_BUFFER,
        //        instanceCap * sizeof(InstanceData),
        //        nullptr,
        //        GL_DYNAMIC_DRAW);
        //}
        //// Upload only the live slice; the rest of the allocation is unused.
        //glBufferSubData(GL_ARRAY_BUFFER,
        //    0,
        //    needed * sizeof(InstanceData),
        //    instances.data());

        //QUESTION THIS!
        glBufferData(GL_ARRAY_BUFFER, needed * sizeof(InstanceData), instances.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void InstanceBatch::Shutdown()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &instanceVbo);
        vao = instanceVbo = instanceCap = 0;
    }
}

namespace Graphics
{
    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    bool Renderer::Init()
    {
        Debug::CLog("Initializing Renderer...\n");
        //_commandBuffer.reserve(8192 * 4);
        _batches.reserve(64);
        Debug::CLog("Renderer initialized successfully\n");
        return true;
    }

    void Renderer::Shutdown()
    {
        for (auto& [key, batch] : _batches)
            batch.Shutdown();
        _batches.clear();
    }

    void Renderer::Begin()
    {
        //_commandBuffer.clear(); // discard last frame's commands; does not free the vector's memory
        for (auto& [key, batch] : _batches)
            batch.instances.clear();
    }

    void Renderer::Queue(const DrawCommand& cmd)
    {
        BatchKey key{ cmd.materialHandle, cmd.meshHandle };
        if (_batches.find(key) == _batches.end())
        {
            if (!Asset::HasMaterial(cmd.materialHandle) || !Asset::HasMesh(cmd.meshHandle))
            {
                Debug::LogWarning("Renderer::Queue: unknown material or mesh handle, skipping\n");
                return;
            }

            Asset::Mesh& mesh = Asset::GetMesh(cmd.meshHandle);
            InstanceBatch batch;
            batch.Init(mesh.vao, mesh.vbo, mesh.ibo);
            _batches.emplace(key, std::move(batch));
        }

        InstanceData inst;
        inst.position = cmd.position;
        inst.rotation = cmd.rotation;
        inst.scale = cmd.scale;
        inst._pad = 0.f;
        inst.tint = cmd.tint;

        _batches[key].instances.push_back(inst);
        //_commandBuffer.push_back(cmd); // no GPU work yet
    }

    void Renderer::End()
    {
        //PROFILE_FUNCTION();
        //Sort();
        Flush();
    }

    void Renderer::SetCamera(const mat4& viewProjection)
    {
        _viewProjection = viewProjection;
    }

    // -----------------------------------------------------------------------
    // Private
    // -----------------------------------------------------------------------

    void Renderer::Flush()
    {
        PROFILE_FUNCTION();

        struct BatchRef
        {
            const BatchKey* key;
            InstanceBatch* batch;
            const Asset::Material* mat;
        };

        std::vector<BatchRef> sorted;
        sorted.reserve(_batches.size());

        for (auto& [key, batch] : _batches)
        {
            if (batch.instances.empty()) continue;
            if (!Asset::HasMaterial(key.materialHandle)) continue;

            sorted.push_back({ &key, &batch, &Asset::GetMaterial(key.materialHandle) });
        }

        if (sorted.empty()) return;

        std::sort(sorted.begin(), sorted.end(),
            [](const BatchRef& a, const BatchRef& b)
            {
                if (a.mat->layer != b.mat->layer) return a.mat->layer < b.mat->layer;
                return a.mat->shaderHandle < b.mat->shaderHandle;
            });

        const Asset::AssetHandle* boundShader = nullptr;

        for (BatchRef& ref : sorted)
        {
            const Asset::Material& mat = *ref.mat;
            Asset::Shader& shader = Asset::GetShader(mat.shaderHandle);
            Asset::Texture2D& tex = Asset::GetTexture(mat.textureHandle);
            Asset::Mesh& mesh = Asset::GetMesh(ref.key->meshHandle);

            // Upload instance data to GPU before drawing.
            ref.batch->Upload();

            if (!boundShader || *boundShader != mat.shaderHandle)
            {
                shader.Bind();
                shader.SetMat4("uViewProjection", _viewProjection);
                boundShader = &mat.shaderHandle;
            }

            shader.SetTexture("uTexture", tex);

            glBindVertexArray(ref.batch->vao);

            glDrawElementsInstanced(
                GL_TRIANGLES,
                (int)mesh.indexCount,
                GL_UNSIGNED_INT,
                nullptr,
                (int)ref.batch->instances.size()
            );
        }

        glBindVertexArray(0);
    }
}