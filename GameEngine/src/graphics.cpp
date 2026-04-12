#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "debug.hpp"

// -------------------------------------------------------------------
// public API
// -------------------------------------------------------------------

bool Renderer::Init()
{
    Debug::CLog("Initializing Renderer...\n");
    Debug::CLog("Renderer initialized successfully\n");
    return true;
}

void Renderer::Shutdown()
{

}

void Renderer::Begin()
{
    _commandBuffer.clear();
}

void Renderer::Queue(DrawCommand cmd)
{
    _commandBuffer.push_back(cmd);
}

void Renderer::End()
{
    Sort();
    Flush();
}

void Renderer::SetCamera(const mat4& viewProjection)
{
    _viewProjection = viewProjection;
}

// -------------------------------------------------------------------
// private
// -------------------------------------------------------------------

void Renderer::Sort()
{
    // placeholder - sort by depth / shader / texture later
    std::sort(_commandBuffer.begin(), _commandBuffer.end(),
        [](const DrawCommand& a, const DrawCommand& b)
        {
            return a.shaderName < b.shaderName;
        });
}

void Renderer::Flush()
{
    const std::string* boundShader = nullptr;
    const std::string* boundMesh = nullptr;

    for (const DrawCommand& cmd : _commandBuffer)
    {
        if (!boundShader || *boundShader != cmd.shaderName)
        {
            ShaderLibrary::Get().Get(cmd.shaderName).Bind();
            boundShader = &cmd.shaderName;

            //set view proj
            ShaderLibrary::Get().Get(cmd.shaderName).SetMat4("uViewProjection", _viewProjection);
        }

        // bind mesh if changed
        if (!boundMesh || *boundMesh != cmd.meshName)
        {
            glBindVertexArray(MeshLibrary::Get().Get(cmd.meshName).vao);
            boundMesh = &cmd.meshName;
        }

        Shader& shader = ShaderLibrary::Get().Get(cmd.shaderName);

        mat4 model = mat4(1.0f);
        model = glm::translate(model, cmd.position);
        model = glm::rotate(model, cmd.rotation, float3(0.f, 0.f, 1.f));
        model = glm::scale(model, cmd.size);

        shader.SetMat4("uModel", model);

        Mesh& mesh = MeshLibrary::Get().Get(cmd.meshName);
        glDrawElements(GL_TRIANGLES, (int)mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}