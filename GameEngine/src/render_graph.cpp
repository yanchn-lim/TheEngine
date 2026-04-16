#include "render_graph.hpp"

namespace Graphics
{
	namespace RenderGraph
	{
		ResourceHandle RenderGraph::CreateTexture(const std::string& name, const TextureDesc& desc)
		{
			ResourceHandle handle = static_cast<ResourceHandle>(_resources.size());
			ResourceInfo info;
			info.type = ResourceInfo::Type::TEXTURE;
		}
	}
}