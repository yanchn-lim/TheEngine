#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <stb_image.h>

#include "debug.hpp"
#include "texture.hpp"


namespace Asset
{
	bool Texture2D::Upload(const std::string& path)
		{
			int w{}, h{}, c{};
			
			//loading data in
			unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
			if (!data)
			{
				Debug::CLog("Failed to load texture : (", path, ")\n");
				return false;
			}

			width = static_cast<uint>(w);
			height = static_cast<uint>(h);
			channels = static_cast<uint>(c);

			glGenTextures(1, &id); //create a texture id
			glBindTexture(GL_TEXTURE_2D, id);

			//set texture params
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			//upload into gpu
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			//free cpu mem
			stbi_image_free(data);

			return true;
		}

	void Texture2D::Shutdown()
	{
		//opengl release mem
		glDeleteTextures(1,&id);
	}

}
