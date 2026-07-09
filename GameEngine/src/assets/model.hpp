#pragma once

#include "asset_handle.hpp"
#include <vector>
namespace Assets
{
	struct Model
	{
		std::vector<MeshHandle> meshes;
	};
}