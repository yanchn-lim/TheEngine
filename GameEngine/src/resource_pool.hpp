#pragma once

#include "render_graph_types.hpp"

namespace Graphics
{
	namespace RenderGraph
	{
		class ResourcePool
		{
		public:
			//get based off descriptors/release back into pool
			GLuint AcquireTexture(const TextureDesc& desc);
			void ReleaseTexture(GLuint id, const TextureDesc& desc);

			//same thing for buffers
			GLuint AcquireBuffer(const BufferDesc& desc);
			void ReleaseBuffer(GLuint id, const BufferDesc& desc);

			//end of frame, destroy old unused resource
			void GarbageCollect();
		private:
			struct PoolEntry
			{
				GLuint id;
				uint lastUsedFrame;
			};

			// Map from descriptor to a list of available textures
			std::unordered_map<TextureDesc, std::vector<PoolEntry>> _texturePool;
			std::unordered_map<BufferDesc , std::vector<PoolEntry>> _bufferPool;

			uint _currentFrame = 0;
		};
	}
}