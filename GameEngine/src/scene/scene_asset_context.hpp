#pragma once

#include <string>
#include <unordered_map>

#include "assets/asset_handle.hpp"

namespace Ludus
{
	// aliases are local to one scene, but their handles refer to assets owned
	// by the shared AssetManager.
	struct SceneAssetContext
	{
		std::unordered_map<std::string, Ludus::Assets::MaterialHandle> materials;
		std::unordered_map<std::string, Ludus::Assets::MeshHandle> meshes;
		std::unordered_map<std::string, bool> meshHasDefaultMaterials;
	};
}
