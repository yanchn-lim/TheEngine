#pragma once

#include <cstdint>
#include <string_view>

#include "debug/memory_tracker.hpp"

namespace Graphics
{
	class Texture2D
	{
	private:
		uint32_t _id{};
		int _width{};
		int _height{};
		int _channels{};
		Memory::ResourceUsage _memoryUsage;

	public:
		bool LoadFromFile(const char* path, std::string_view label = {});
		bool CreateFromRGBA(const unsigned char* pixels, int width, int height, std::string_view label = "Generated Texture");
		void Bind(uint32_t slot = 0) const;
		void Destroy();

		bool IsValid() const;

		//accessor
		int GetWidth() const;
		int GetHeight() const;
		int GetChannels() const;

		//ctor
		Texture2D() = default;
		~Texture2D();
		Texture2D(const Texture2D&) = delete;
		Texture2D& operator=(const Texture2D&) = delete;

		Texture2D(Texture2D&&) noexcept;
		Texture2D& operator=(Texture2D&&) noexcept;
	};
}
