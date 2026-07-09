#pragma once

#include "asset_handle.hpp"

#include <vector>

namespace Assets
{
	struct ModelAsset
	{
		std::vector<MeshHandle> meshes;
	};
}
