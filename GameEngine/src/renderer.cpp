#include <glad/glad.h>

#include "debug.hpp"
#include "profiler.hpp"

#include "renderer.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"

namespace Graphics
{
    mat4 DrawCommand::SetModel(float3 position, float3 size, float rotation)
    {
        mat4 model = mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation, float3(0.f, 0.f, 1.f));
        model = glm::scale(model, size);
        return model;
    }
    

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    bool Renderer::Init()
    {
        Debug::CLog("Initializing Renderer...\n");
        _commandBuffer.reserve(8192 * 4);
        Debug::CLog("Renderer initialized successfully\n");
        return true;
    }

    void Renderer::Shutdown()
    {

    }

    void Renderer::Begin()
    {
        _commandBuffer.clear(); // discard last frame's commands; does not free the vector's memory
    }

    void Renderer::Queue(DrawCommand&& cmd)
    {
        _commandBuffer.emplace_back(std::move(cmd)); // no GPU work yet
    }

    void Renderer::Queue(const DrawCommand& cmd)
    {
        _commandBuffer.push_back(cmd); // no GPU work yet
    }

    void Renderer::End()
    {
        //PROFILE_FUNCTION();
        Sort();
        Flush();
    }

    void Renderer::SetCamera(const mat4& viewProjection)
    {
        _viewProjection = viewProjection;
    }

    // -----------------------------------------------------------------------
    // Private
    // -----------------------------------------------------------------------

    void Renderer::Sort()
    {
        PROFILE_FUNCTION();

        // Group commands by shader name so we minimise expensive glUseProgram switches.
        std::sort(_commandBuffer.begin(), _commandBuffer.end(),
            [](const DrawCommand& a, const DrawCommand& b)
            {
                const Resource::Material& ma = Resource::MaterialLibrary::Get().Get(a.materialHandle);
                const Resource::Material& mb = Resource::MaterialLibrary::Get().Get(b.materialHandle);
                if (ma.layer != mb.layer) return ma.layer < mb.layer;
                return ma.shaderHandle < mb.shaderHandle;
            });
    }

    void Renderer::Flush()
    {
        PROFILE_FUNCTION();

        // Track what is currently bound to skip redundant bind calls.
        const AssetHandle* boundShader = nullptr;
        const AssetHandle* boundMesh = nullptr;

        Graphics::Resource::MaterialLibrary& matLib = Graphics::Resource::MaterialLibrary::Get();


        for (const DrawCommand& cmd : _commandBuffer)
        {
            // get material
            if (!matLib.Has(cmd.materialHandle))
            {
                Debug::CLog("Failed to get material : ", cmd.materialHandle, "! Skipping...\n");
                continue;
            }

            Resource::Material& mat = matLib.Get(cmd.materialHandle);
            Resource::Shader& shader = Resource::GetShader(mat.shaderHandle);
            Resource::Texture& tex = Resource::GetTexture(mat.textureHandle);
            Resource::Mesh& mesh = Resource::GetMesh(cmd.meshHandle);

            //bind shader only when changed
            if (!boundShader || *boundShader != mat.shaderHandle)
            {
                shader.Bind();
                shader.SetMat4("uViewProjection", _viewProjection);
                boundShader = &mat.shaderHandle;
            }

            // Binding the VAO restores all the buffer and attribute-pointer state set up in Mesh::Init().
            if (!boundMesh || *boundMesh != cmd.meshHandle)
            {
                glBindVertexArray(mesh.vao);
                boundMesh = &cmd.meshHandle;
            }

            shader.SetMat4("uModel", cmd.model);
            shader.SetTexture("uTexture", tex);

            // Draw using the IBO bound inside the VAO; nullptr = start from the beginning of the index buffer.
            glDrawElements(GL_TRIANGLES, (int)mesh.indexCount, GL_UNSIGNED_INT, nullptr);

            //glDrawArraysInstanced();
        }

        glBindVertexArray(0);
    }
}