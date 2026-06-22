#pragma once

#include "transform2d.hpp"
#include "sprite_component.hpp"

class GameObject
{
public:
	bool enabled{ true };
	Transform2D transform;
	SpriteComponent sprite;
};