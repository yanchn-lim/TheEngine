#pragma once

#include "asset_handle.hpp"
#include "asset_records.hpp"

namespace Ludus::Assets
{
	// stores source paths for the OpenGL and Vulkan variants of each shader
	class ShaderRegistry
	{
	public:
		ShaderHandle Load(const std::string& vertexPath, const std::string& fragmentPath,
			const std::string& vertexSpirvPath = {}, const std::string& fragmentSpirvPath = {});
		const ShaderAsset* Get(ShaderHandle handle) const;
		void Clear();

	private:
		AssetId _nextId = 1;

		std::unordered_map<std::string, ShaderHandle> _pathToHandle;
		std::unordered_map<AssetId, ShaderAsset> _shaders;
	};
}
