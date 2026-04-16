// pass_builder.hpp
#pragma once

#include "render_graph_types.hpp"
#include "render_graph_pass.hpp"

namespace Graphics
{
    namespace RenderGraph {

        class RenderGraph; //fwd decl

        class PassBuilder 
        {
        public:
            PassBuilder(RenderGraph* graph, RenderPass* pass);

            // Declare that this pass reads a resource (adds to pass.reads and graph edges)
            PassBuilder& Read(ResourceHandle resource);

            // Declare a color attachment (write) with optional clear
            PassBuilder& WriteColor(ResourceHandle resource,
                                    GLenum loadOp = GL_LOAD,
                                    const glm::vec4& clearColor = glm::vec4(0.0f));

            // Declare depth/stencil attachment
            PassBuilder& WriteDepth(ResourceHandle resource,
                                    GLenum loadOp = GL_LOAD,
                                    float clearDepth = 1.0f,
                                    GLenum storeOp = GL_STORE);
            
            // Set the execution callback
            PassBuilder& SetExecute(std::function<void(RenderContext&)> exec);

        private:
            RenderGraph* _graph;
            RenderPass* _pass;
        };
    }
}