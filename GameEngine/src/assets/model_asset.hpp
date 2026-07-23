#pragma once

#include "asset_handle.hpp"

#include <vector>

namespace Assets
{
	// a model groups mesh assets without owning graphics resources
	struct ModelAsset
	{
		std::vector<MeshHandle> meshes;
	};
}
