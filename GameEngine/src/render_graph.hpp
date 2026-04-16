// render_graph.hpp
#pragma once

#include "resource_pool.hpp"
#include "pass_builder.hpp"

namespace Graphics
{
    namespace RenderGraph {

        class RenderContext; //fwd decl

        class RenderGraph 
        {
        public:
            RenderGraph();
            ~RenderGraph();

            // --------------------------------------------------------------------
            // Resource Creation / Import
            // --------------------------------------------------------------------
            ResourceHandle CreateTexture(const std::string& name, const TextureDesc& desc);
            ResourceHandle CreateBuffer(const std::string& name, const BufferDesc& desc);

            // Import an existing OpenGL texture (e.g., backbuffer, external FBO)
            ResourceHandle ImportTexture(const std::string& name, GLuint existingID, const TextureDesc& desc);
            ResourceHandle ImportBuffer(const std::string& name, GLuint existingID, const BufferDesc& desc);

            // Retrieve info about a resource (for debugging)
            const ResourceInfo* GetResourceInfo(ResourceHandle handle) const;

            // --------------------------------------------------------------------
            // Pass Building
            // --------------------------------------------------------------------
            // Add a pass. The lambda receives a PassBuilder to configure the pass.
            void AddPass(const std::string& name, std::function<void(PassBuilder&)> setup);

            // --------------------------------------------------------------------
            // Compilation & Execution
            // --------------------------------------------------------------------
            void Compile();   // Builds dependency graph, sorts passes, allocates transient resources
            void Execute();   // Executes all passes in order, managing FBOs and resource binding
            void Clear();     // Resets the graph for next frame

            // For debugging: print graph structure
            void DebugPrint() const;

        private:
            friend class RenderContext;
            friend class PassBuilder;

            // Resource storage
            std::vector<ResourceInfo> _resources;
            std::unordered_map<std::string, ResourceHandle> _resourceNameMap;

            // Pass storage
            std::vector<RenderPass> _passes;

            // Adjacency for dependencies (built during Compile)
            std::vector<std::vector<uint32_t>> _passDependencies; // indices of passes this pass depends on

            // Execution order (topological sort result)
            std::vector<uint32_t> _executionOrder;

            // Resource pool for transient allocations
            ResourcePool _resourcePool;

            // During compilation: track writer per resource
            std::vector<int> _resourceWriter; // pass index or -1

            // Internal methods
            void BuildDependencies();
            void TopologicalSort();
            void ComputeResourceLifetimes();
            void AllocateTransientResources();
            void CreateFramebuffers(); // For each pass, build FBO with acquired textures
            void ExecutePass(uint passIndex);

            // For execution: mapping ResourceHandle -> actual GL texture ID for this frame
            struct FrameResource 
            {
                GLuint glID;
                TextureDesc desc;
            };

            std::vector<FrameResource> _frameResources; // indexed by ResourceHandle (for transient/imported)
            std::vector<GLuint> _passFramebuffers;      // one per pass, created during Compile
        };
    }
}