#pragma once

#include "graphics_resource.hpp"
// material => { shader, texture }
// material applied onto mesh
// render

namespace Graphics
{
    // Plain data describing one object to draw. No GPU work happens here -
    // commands are collected each frame, sorted, then flushed all at once.
    struct DrawCommand
    {
        AssetHandle materialHandle; //key into material lib
        AssetHandle meshHandle;   // key into MeshLibrary
        int layer;

        float3 position;
        float3 scale;
        float rotation;

        static mat4 SetModel(float3 position, float3 size, float rotation);
    };

    // Collects DrawCommands, sorts them to minimise GPU state changes, then issues draw calls.
    // Usage per frame: Begin() -> SetCamera() -> Queue() -> End().
    class Renderer
    {
    public:
        static Renderer& Get() { static Renderer instance; return instance; }
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Init();
        void Shutdown();

        void Begin();              // clears the command buffer
        void End();                // sorts and flushes all queued commands to the GPU
        void Queue(DrawCommand&& cmd);
        void Queue(const DrawCommand& cmd);

        void SetCamera(const mat4& viewProjection); // stores the VP matrix; uploaded to each shader in Flush()

    private:
        Renderer() = default;
        std::vector<DrawCommand> _commandBuffer{};
        mat4 _viewProjection = mat4(1.f);

        void Sort();  // sorts by shader name to reduce glUseProgram calls
        void Flush(); // iterates sorted commands and calls glDrawElements for each
    };
}



