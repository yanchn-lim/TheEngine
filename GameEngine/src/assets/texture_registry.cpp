#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "texture_registry.hpp"

#include "debug/debug.hpp"

namespace Ludus::Assets
{
    TextureHandle TextureRegistry::Load(const std::string& path)
    {
        // decode each source path once and keep a CPU copy in standard RGBA order
        if (const auto existing = _pathToHandle.find(path); existing != _pathToHandle.end())
            return existing->second;

        stbi_set_flip_vertically_on_load(true);
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* source = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!source || width <= 0 || height <= 0)
        {
            Ludus::Debug::LogError("TextureRegistry::Load : Failed to load texture ", path);
            stbi_image_free(source);
            return {};
        }

        // copy stb_image memory into registry-owned storage before releasing it
        TextureAsset asset;
        asset.width = static_cast<uint32_t>(width);
        asset.height = static_cast<uint32_t>(height);
        asset.label = path;
        const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        asset.pixels.assign(source, source + byteCount);
        stbi_image_free(source);

        const TextureHandle handle{ _nextId++ };
        _pathToHandle[path] = handle;
        _textures.emplace(handle.id, std::move(asset));
        return handle;
    }

	TextureHandle TextureRegistry::CreateSolidColor(
		const std::string& name,
		unsigned char red,
		unsigned char green,
		unsigned char blue,
		unsigned char alpha)
	{
		if (const auto existing = _pathToHandle.find(name);
			existing != _pathToHandle.end())
		{
			return existing->second;
		}

		TextureAsset asset;
		asset.pixels = { red, green, blue, alpha };
		asset.width = 1;
		asset.height = 1;
		asset.label = name;

		const TextureHandle handle{ _nextId++ };
		_pathToHandle[name] = handle;
		_textures.emplace(handle.id, std::move(asset));
		return handle;
	}

    const TextureAsset* TextureRegistry::Get(TextureHandle handle) const
    {
        if (!handle)
            return nullptr;

        const auto found = _textures.find(handle.id);
        return found == _textures.end() ? nullptr : &found->second;
    }

    void TextureRegistry::Clear()
    {
        _nextId = 1;
        _pathToHandle.clear();
        _textures.clear();
    }
}
