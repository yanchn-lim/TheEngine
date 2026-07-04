#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>

#include "texture2d.hpp"
#include "debug/debug.hpp"

namespace Graphics
{
	bool Texture2D::LoadFromFile(const char* path)
	{
		Destroy(); //make sure to remove first

		int width = 0;
		int height = 0;
		int channels = 0;
		
		stbi_set_flip_vertically_on_load(true);

		unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
		
		if (!data)
		{
			Debug::LogError("Failed to load image : ", path);
			return false;
		}

		//upload to gpu
		GLenum format = GL_RGB; //default

		if (channels == 4)
			format = GL_RGBA;
		else if (channels == 3)
			format = GL_RGB;
		else //invalid channels
		{
			Debug::LogError("Failed to load image : Unexpected channels ", "(",channels,")");
			stbi_image_free(data);
			return false;
		}

		//create
		glCreateTextures(GL_TEXTURE_2D, 1 ,&_id);

		//set params
		glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//determine how to store texture
		glTextureStorage2D(_id, 1, format == GL_RGBA ? GL_RGBA8 : GL_RGB8, width, height);
		//pass in the data
		glTextureSubImage2D(_id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

		//generate mipmap
		//glGenerateTextureMipmap(_id);

		_width = width;
		_height = height;
		_channels = channels; // 1 = grayscale, 3 = rgb, 4 = rgba

		stbi_image_free(data);

		return true;
	}

	bool Texture2D::CreateFromRGBA(const unsigned char* pixels, int width, int height)
	{
		Destroy();

		if (!pixels)
		{
			Debug::LogError("Texture2D::CreateFromRGBA failed: pixels is null");
			return false;
		}

		if (width <= 0 || height <= 0)
		{
			Debug::LogError("Texture2D::CreateFromRGBA failed: invalid dimensions ", width, "x", height);
			return false;
		}

		glCreateTextures(GL_TEXTURE_2D, 1, &_id);

		glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureStorage2D(_id, 1, GL_RGBA8, width, height);
		glTextureSubImage2D(_id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		_width = width;
		_height = height;
		_channels = 4;

		return true;
	}

	void Texture2D::Bind(uint32_t slot) const
	{
		if (_id == 0)
		{
			Debug::LogError("Texture2D::Bind failed: texture is invalid");
			return;
		}

		glBindTextureUnit(slot, _id);
	}

	void Texture2D::Destroy()
	{
		if (_id == 0)
			return;
		
		glDeleteTextures(1, &_id);
		_id = 0;
		_width = 0;
		_height = 0;
		_channels = 0;
	}

	bool Texture2D::IsValid() const
	{
		return _id != 0;
	}

	int Texture2D::GetWidth() const 
	{
		return _width;
	}

	int Texture2D::GetHeight() const 
	{
		return _height;
	}

	int Texture2D::GetChannels() const 
	{
		return _channels;
	}

	Texture2D::~Texture2D()
	{
		Destroy();
	}

	Texture2D::Texture2D(Texture2D&& oth) noexcept : 
		_id(oth._id), 
		_width(oth._width), _height(oth._height), 
		_channels(oth._channels)
	{
		oth._id = 0;
		oth._width = 0;
		oth._height = 0;
		oth._channels = 0;
	}

	Texture2D& Texture2D::operator=(Texture2D&& oth) noexcept
	{
		if (&oth == this) return *this;

		Destroy();

		_id = oth._id;
		_width = oth._width;
		_height = oth._height;
		_channels = oth._channels;

		oth._id = 0;
		oth._width = 0;
		oth._height = 0;
		oth._channels = 0;

		return *this;
	}
}
