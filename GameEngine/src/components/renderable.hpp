#pragma once

#include "assets/asset_handle.hpp"

namespace Ludus::Components
{
	struct Renderable
	{
		Ludus::Assets::MeshHandle mesh;
		// when valid, this replaces every surface material
		Ludus::Assets::MaterialHandle materialOverride;
		bool visible = true;
	};
}
