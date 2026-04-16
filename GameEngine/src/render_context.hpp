#pragma once

#include "render_graph_types.hpp"

namespace Graphics
{
	namespace RenderGraph
	{
		class RenderGraph; //fwd decl
		struct RenderPass;

		class RenderContext
		{
		public:
			RenderContext(RenderGraph* graph, const RenderPass* pass);

			//get opengl texture id
			GLuint GetTexture(ResourceHandle handle) const;
			glm::uvec2 GetTextureSize(ResourceHandle handle) const;

			void SetViewPort(int x, int y, int width, int height);

			//bind a uniform buffer for a resource
			GLuint GetBuffer(ResourceHandle handle) const;

			GLuint GetFrameBuffer() const { return _frameBuffer; }

		private:
			RenderGraph* _graph;
			const RenderPass* _pass;
			GLuint _frameBuffer = 0;
		};
	}
}