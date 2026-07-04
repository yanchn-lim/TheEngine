#include "debug/debug.hpp"
#include "shader_registry.hpp"
#include "core/file_system.hpp"

namespace Assets
{
	namespace
	{
		const char* FallbackVertexShader = R"(
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out vec2 vTexCoord;

void main()
{
	vColor = aColor;
	vTexCoord = aTexCoord;
	gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

		const char* FallbackFragmentShader = R"(
#version 460 core

in vec3 vColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
	vec4 checker = texture(uTexture, vTexCoord);
	FragColor = checker * vec4(vColor, 1.0);
}
)";
	}

	ShaderHandle ShaderRegistry::Load(const std::string& vertexPath, const std::string& fragmentPath)
	{
		std::string concatPath = vertexPath + "|" + fragmentPath;
		const auto it = _pathToHandle.find(concatPath);

		if (it != _pathToHandle.end())
			return it->second;

		std::string vertSrc;
		std::string fragSrc;
		if (!FileSystem::ReadTextFile(vertexPath.c_str(), vertSrc))
		{
			Debug::LogError("ShaderRegistry::Load : Failed to read vertex shader ", vertexPath, ". Using fallback shader for invalid lookups.");
			return ShaderHandle();
		}

		if (!FileSystem::ReadTextFile(fragmentPath.c_str(), fragSrc))
		{
			Debug::LogError("ShaderRegistry::Load : Failed to read fragment shader ", fragmentPath, ". Using fallback shader for invalid lookups.");
			return ShaderHandle();
		}

		Graphics::Shader shader;
		if (!shader.Create(vertSrc, fragSrc))
		{
			Debug::LogError("ShaderRegistry::Load : Failed to create shader program. Using fallback shader for invalid lookups.");
			return ShaderHandle();
		}
		
		ShaderHandle handle{ _nextId++ };
		_pathToHandle[concatPath] = handle;
		_shaders[handle.id] = std::move(shader);

		return handle;
	}

	const Graphics::Shader* ShaderRegistry::Get(ShaderHandle handle) const
	{
		if (!handle)
		{
			Debug::LogError("ShaderRegistry::Get : ShaderHandle [", handle.id, "] is invalid. Returning fallback shader.");
			return GetFallback();
		}

		const auto it = _shaders.find(handle.id);
		if (it == _shaders.end())
		{
			Debug::LogError("ShaderRegistry::Get : Could not find ShaderHandle [", handle.id, "] in the registry. Returning fallback shader.");
			return GetFallback();
		}


		return &it->second;
	}

	const Graphics::Shader* ShaderRegistry::GetFallback() const
	{
		if (_fallbackShaderReady && _fallbackShader.IsValid())
			return &_fallbackShader;

		if (!_fallbackShader.Create(FallbackVertexShader, FallbackFragmentShader))
		{
			Debug::LogError("ShaderRegistry::GetFallback : Failed to create fallback shader");
			return nullptr;
		}

		_fallbackShaderReady = true;
		return &_fallbackShader;
	}

	void ShaderRegistry::Clear()
	{
		_nextId = 1;
		_pathToHandle.clear();
		_shaders.clear();
		_fallbackShader.Destroy();
		_fallbackShaderReady = false;
	}
}
