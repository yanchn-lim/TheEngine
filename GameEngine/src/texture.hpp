#pragma once


namespace Asset
{
	class Texture2D
	{
	public:
		uint id = 0; //gl texture handle
		uint width = 0;
		uint height = 0;
		uint channels = 0;

		bool Upload(const std::string& path); //upload to gpu
		bool IsValid() const { return id != 0; } //0 is invalid
		void Shutdown(); //delete textures
	};
}
