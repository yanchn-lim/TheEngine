#pragma once

#include "asset_handle.hpp"
#include "graphics/shader.hpp"

namespace Assets
{
	class ShaderRegistry
	{
	public:
		ShaderHandle Load(const std::string& vertexPath, const std::string& fragmentPath);
		const Graphics::Shader* Get(ShaderHandle handle) const;
		void Clear();

	private:
		const Graphics::Shader* GetFallback() const;

		AssetId _nextId = 1;

		std::unordered_map<std::string, ShaderHandle> _pathToHandle;
		std::unordered_map<AssetId, Graphics::Shader> _shaders;

		mutable Graphics::Shader _fallbackShader;
		mutable bool _fallbackShaderReady = false;
	};
}
