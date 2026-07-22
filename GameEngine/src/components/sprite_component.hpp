#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include "assets/asset_handle.hpp"
#include "rendering/render_world.hpp"

struct SpriteComponent
{
	// serialized fields describe sprite appearance and ordering
	bool visible{ true };
	Assets::TextureHandle texture;
	Assets::MaterialHandle material;
	glm::vec4 color{ 1.f };
	glm::vec2 uvMin{ 0.f, 0.f };
	glm::vec2 uvMax{ 1.f, 1.f };
	glm::vec2 pivot{ 0.5f, 0.5f };
	int sortingOrder{ 0 };
	// runtime fields connect this component to RenderWorld and are not serialized
	Rendering::RenderInstanceHandle renderInstance;
	uint64_t renderVersion = 0;

	bool HasTexture() const
	{
		return static_cast<bool>(texture);
	}
};
