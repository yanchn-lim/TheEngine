#pragma once

#include "assets/asset_handle.hpp"

namespace Components
{
	struct Renderable
	{
		Assets::MeshHandle mesh;
		// when valid, this replaces every surface material
		Assets::MaterialHandle materialOverride;
		bool visible = true;
	};
}
