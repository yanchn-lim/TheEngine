#pragma once

#include "asset.hpp"
#include "asset_types.hpp"

namespace Graphics
{
    // Plain data describing one object to draw. No GPU work happens here -
    // commands are collected each frame, sorted, then flushed all at once.
    struct DrawCommand
    {
        Asset::AssetHandle materialHandle; //key into material lib
        Asset::AssetHandle meshHandle;   // key into MeshLibrary
   
        float3 position{ 0.f, 0.f, 0.f };
        float3 scale{ 1.f, 1.f, 1.f };
        float  rotation{ 0.f };           // radians, Z axis
        float4 tint{ 1.f, 1.f, 1.f, 1.f };

        DrawCommand() = default;
    };

    struct InstanceData
    {
        float3 position;
        float rotation;
        float3 scale;
        float _pad; //pad to ensure vec4 is preserved
        float4 tint;
    };

    struct InstanceBatch
    {
        uint vao{ 0 };  // combined geometry + instance attribs
        uint instanceVbo{ 0 };  // stream-updated per frame
        uint instanceCap{ 0 };  // current allocated capacity on the GPU (in instance count)

        std::vector<InstanceData> instances{};

        // Creates vao/instanceVbo and wires up the instance attribute pointers
        // against the already-uploaded mesh vao/vbo.
        void Init(uint meshVao, uint meshVbo, uint meshIbo);

        // Uploads instances to GPU, resizes the instanceVbo if needed.
        void Upload();

        void Shutdown();
    };

    struct BatchKey
    {
        Asset::AssetHandle materialHandle;
        Asset::AssetHandle meshHandle;

        bool operator==(const BatchKey& o) const
        {
            return materialHandle == o.materialHandle &&
                meshHandle == o.meshHandle;
        }
    };

}

namespace std
{
    template<>
    struct hash<Graphics::BatchKey>
    {
        size_t operator()(const Graphics::BatchKey& k) const noexcept
        {
            size_t h1 = std::hash<Asset::AssetHandle>{}(k.materialHandle);
            size_t h2 = std::hash<Asset::AssetHandle>{}(k.meshHandle);
            return h1 ^ (h2 << 32 | h2 >> 32); // mix the two hashes
        }
    };
}

namespace Graphics
{
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
        void Queue(const DrawCommand& cmd);

        void SetCamera(const mat4& viewProjection); // stores the VP matrix; uploaded to each shader in Flush()

    private:
        Renderer() = default;
        std::unordered_map<BatchKey, InstanceBatch> _batches{};
        mat4 _viewProjection = mat4(1.f);

        void Flush(); // iterates sorted commands and calls glDrawElements for each
    };
}



