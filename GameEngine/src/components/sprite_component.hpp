#pragma once

#include <cstdint>

#include <glm/glm.hpp>

using TextureHandle = uint32_t;

constexpr TextureHandle InvalidTexture = 0;

struct SpriteComponent
{
	bool visible{ true };
	TextureHandle texture{ InvalidTexture };
	glm::vec4 color{ 1.f };
	glm::vec2 uvMin{ 0.f, 0.f };
	glm::vec2 uvMax{ 1.f, 1.f };
	glm::vec2 pivot{ 0.5f, 0.5f };
	int sortingOrder{ 0 };

	bool HasTexture() const
	{
		return texture != InvalidTexture;
	}
};
