#pragma once

#include <glm/glm.hpp>

struct Transform2D
{
	glm::vec2 position{ 0.f };
	glm::vec2 scale{ 1.f };
	float rotation{ 0.f }; // radians

	glm::mat4 GetMatrix() const;
};
