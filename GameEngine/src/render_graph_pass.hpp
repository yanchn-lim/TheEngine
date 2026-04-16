#pragma once

#include "render_graph_types.hpp"

namespace Graphics
{
	namespace RenderGraph
	{
		class RenderContext; //fwd decl

		enum class AttachmentLoadOp 
		{
			Load,       // Preserve existing contents (do nothing)
			Clear,      // Clear to a specified value
			DontCare    // Contents are undefined (we can still skip clear)
		};

		// Optional: StoreOp for completeness (can be added later for discarding)
		enum class AttachmentStoreOp 
		{
			Store,      // Keep contents (default)
			DontCare    // Discard (may improve performance on tile-based GPUs)
		};

		struct AttachmentInfo
		{
			ResourceHandle resource;
			AttachmentLoadOp loadOp = AttachmentLoadOp::Load;
			AttachmentStoreOp storeOp = AttachmentStoreOp::Store; // not used yet
			union 
			{
				float clearColor[4];
				float clearDepth;
				int   clearStencil;
			};
		};


		struct RenderPass
		{
			std::string name; //for debug

			//resource being used by this pass (input)
			std::vector<ResourceHandle> reads; 
			//resource being written by this pass (output)
			std::vector<ResourceHandle> writes;

			//framebuffer attachments (color + [depth/stencil/etc...])
			std::vector<AttachmentInfo> colorAttachments;
			AttachmentInfo depthStencilAttachment;

			std::function<void(RenderContext&)> execute;

			//index after sorting
			uint executionIndex = 0;
		};
	}
}