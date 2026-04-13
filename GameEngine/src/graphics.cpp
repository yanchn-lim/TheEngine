#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "debug.hpp"
#include "profiler.hpp"

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

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
    _commandBuffer.clear(); // discard last frame's commands; does not free the vector's memory
}

void Renderer::Queue(DrawCommand cmd)
{
    _commandBuffer.push_back(cmd); // no GPU work yet
}

void Renderer::End()
{
    PROFILE_FUNCTION();
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
            return a.shaderName < b.shaderName;
        });
}

void Renderer::Flush()
{
    PROFILE_FUNCTION();
    // Track what is currently bound to skip redundant bind calls.
    const std::string* boundShader = nullptr;
    const std::string* boundMesh   = nullptr;

    for (const DrawCommand& cmd : _commandBuffer)
    {
        // Rebind shader only when it changes; also re-upload the VP matrix.
        if (!boundShader || *boundShader != cmd.shaderName)
        {
            ShaderLibrary::Get().Get(cmd.shaderName).Bind();
            ShaderLibrary::Get().Get(cmd.shaderName).SetMat4("uViewProjection", _viewProjection);
            boundShader = &cmd.shaderName;
        }

        // Binding the VAO restores all the buffer and attribute-pointer state set up in Mesh::Init().
        if (!boundMesh || *boundMesh != cmd.meshName)
        {
            glBindVertexArray(MeshLibrary::Get().Get(cmd.meshName).vao);
            boundMesh = &cmd.meshName;
        }

        Shader& shader = ShaderLibrary::Get().Get(cmd.shaderName);

        // Build the model matrix: scale -> rotate around Z -> translate to world position.
        mat4 model = mat4(1.0f);
        model = glm::translate(model, cmd.position);
        model = glm::rotate   (model, cmd.rotation, float3(0.f, 0.f, 1.f));
        model = glm::scale    (model, cmd.size);

        shader.SetMat4("uModel", model);

        // Draw using the IBO bound inside the VAO; nullptr = start from the beginning of the index buffer.
        Mesh& mesh = MeshLibrary::Get().Get(cmd.meshName);
        glDrawElements(GL_TRIANGLES, (int)mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}
