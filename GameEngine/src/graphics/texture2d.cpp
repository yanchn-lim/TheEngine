#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "texture2d.hpp"

namespace Graphics
{
	bool Texture2D::LoadFromFile(const char* path)
	{

		stbi_load();
	}

	void Texture2D::Bind(uint32_t slot = 0) const
	{

	}

	void Texture2D::Destroy()
	{

	}

	bool Texture2D::IsValid() const
	{
	}
}