#pragma once

#include "components/transform2d.hpp"
#include "components/sprite_component.hpp"

class GameObject
{
public:
	bool enabled{ true };
	Transform2D transform;
	SpriteComponent sprite;
};
