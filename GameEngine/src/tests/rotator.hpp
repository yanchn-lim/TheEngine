#pragma once

#include <glm/glm.hpp>

namespace Ludus
{
	class SceneComponentRegistry;
}

namespace Tests
{
	struct Rotator
	{
		glm::vec3 axis{ 0.0f, 1.0f, 0.0f };
		float radiansPerSecond = 0.0f;
	};

	void RegisterRotatorSceneComponent(Ludus::SceneComponentRegistry& registry);
}
