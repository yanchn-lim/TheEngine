#pragma once

#include "assets/asset_handle.hpp"

namespace Components
{
	struct Renderable
	{
		Assets::MeshHandle mesh;
		Assets::MaterialHandle material;
		bool visible = true;
	};
}