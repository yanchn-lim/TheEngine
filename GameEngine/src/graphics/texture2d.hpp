#pragma once

namespace Graphics
{
	class Texture2D
	{
	private:
		uint32_t _id{};
		int _width{};
		int _height{};
		int _channels{};

	public:
		bool LoadFromFile(const char* path);
		void Bind(uint32_t slot = 0) const;
		void Destroy();

		bool IsValid() const;
	};
}