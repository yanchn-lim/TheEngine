#pragma once

#include "glad/glad.h"

namespace Graphics
{
	namespace RenderGraph
	{
		using ResourceHandle = uint;
		constexpr ResourceHandle invalidResource = ~0u;

		constexpr ResourceHandle backBuffer = 0;

		struct TextureDesc
		{
			uint width = 0;
			uint height = 0;

			GLenum format = GL_RGBA8; //texture format
			uint mipLevels = 1;
			uint samples = 1;
			bool isCubeMap = false;

			bool operator==(const TextureDesc& other) const
			{
				return width == other.width
					&& height == other.height
					&& format == other.format
					&& mipLevels == other.mipLevels
					&& samples == other.samples
					&& isCubeMap == other.isCubeMap;
			}
		};

		struct BufferDesc
		{
			size_t size;
			GLbitfield flags; //flags (read/write/etc...)
		};

		//stored per handle
		struct ResourceInfo
		{
			enum class Type { TEXTURE, BUFFER, EXTERNAL };
			Type type;
			union
			{
				TextureDesc textureDesc;
				BufferDesc bufferDesc;
			};
			std::string name; //for debugging
			bool isImported = false; //not sure what this does

			//lifetime tracking
			int firstUsePass = -1;
			int lastUsePass = -1;
		};
	}
}