#pragma once

#include "Core/time.hpp"
#include "events.hpp"
#include "ECS/ecs.hpp"

struct GLFWwindow;

namespace Engine
{
	struct EngineCtx
	{
		Time time;
		TimeConfig timeConfig;
		EventBus eventBus;
		ECS::World world;

		//platform
		GLFWwindow* window = nullptr;
		int windowWidth = 1600;
		int windowHeight = 900;

		//add ptrs to systems
		//...

	};
}