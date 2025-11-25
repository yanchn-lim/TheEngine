#pragma once
#include <cstdint>

namespace Engine
{
	struct Time
	{
		float deltaTime = 0.0f;
		float fixedDeltaTime = 1.0f / 60.f;
		float timeSinceStart = 0.0f;
		float accumulator = 0.0f;
		uint64_t frameCount = 0;
	};

	namespace TimeSystem
	{
		void Initialize(Time& time);
		void Update(Time& time);
	}
}