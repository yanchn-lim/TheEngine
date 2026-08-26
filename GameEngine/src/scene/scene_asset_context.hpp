#pragma once

#include <string>
#include <unordered_map>

#include "assets/asset_handle.hpp"

namespace Ludus
{
	struct SceneAssetContext
	{
		std::unordered_map<std::string, Assets::ShaderHandle> shaders;
		std::unordered_map<std::string, Assets::TextureHandle> textures;
		std::unordered_map<std::string, Assets::MaterialHandle> materials;
		std::unordered_map<std::string, Assets::MeshHandle> meshes;
		std::unordered_map<std::string, bool> meshHasDefaultMaterials;
	};
}
