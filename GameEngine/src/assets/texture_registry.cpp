#include "debug/debug.hpp"
#include "texture_registry.hpp"

namespace Assets
{
	TextureHandle Assets::TextureRegistry::Load(const std::string& path)
	{
		//check if alrdy exists
		const auto it = _pathToHandle.find(path);
		if (it != _pathToHandle.end())
			return it->second;

		Graphics::Texture2D tex;
		if (!tex.LoadFromFile(path.c_str()))
		{
			Debug::LogError("TextureRegistry::Load : Failed to load texture from ", path, ". Using fallback texture for invalid lookups.");
			return TextureHandle(); //defaults to invalid
		}

		TextureHandle handle{ _nextId++ };
		_pathToHandle[path] = handle;
		_textures[handle.id] = std::move(tex);

		return handle;
	}

	const Graphics::Texture2D* TextureRegistry::Get(TextureHandle handle) const
	{
		if (!handle)
		{
			Debug::LogError("TextureRegistry::Get : TextureHandle [", handle.id, "] is invalid. Returning fallback texture.");
			return GetFallback();
		}

		const auto it = _textures.find(handle.id);

		if (it == _textures.end())
		{
			Debug::LogError("TextureRegistry::Get : Could not find TextureHandle [", handle.id, "] in the registry. Returning fallback texture.");
			return GetFallback();
		}

		return &it->second;
	}

	const Graphics::Texture2D* TextureRegistry::GetFallback() const
	{
		if (_fallbackTextureReady && _fallbackTexture.IsValid())
			return &_fallbackTexture;

		constexpr unsigned char pixels[] =
		{
			255,   0, 255, 255,   0,   0,   0, 255,
			  0,   0,   0, 255, 255,   0, 255, 255
		};

		if (!_fallbackTexture.CreateFromRGBA(pixels, 2, 2))
		{
			Debug::LogError("TextureRegistry::GetFallback : Failed to create fallback texture");
			return nullptr;
		}

		_fallbackTextureReady = true;
		return &_fallbackTexture;
	}

	void TextureRegistry::Clear()
	{
		_nextId = 1;
		_textures.clear();
		_pathToHandle.clear();
		_fallbackTexture.Destroy();
		_fallbackTextureReady = false;
	}
}
