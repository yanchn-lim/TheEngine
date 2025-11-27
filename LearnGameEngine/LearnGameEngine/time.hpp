#pragma once
#include <cstdint>

namespace Engine
{
	struct TimeConfig
	{
		float fixedDeltaTime = 1.0f / 60.0f;
		float maxDeltaTime = 0.25f; // 250 ms
		float initialTimeScale = 1.0f;
	};

	struct Time
	{
		float deltaTime = 0.0f; //scaled DT
		float unscaledDeltaTime = 0.0f;//unscaled DT
		float fixedDeltaTime = 1.0f / 60.f;//fixed DT
		float timeSinceStart = 0.0f;
		float unscaledTimeSinceStart = 0.0f;
		float accumulator = 0.0f;
		float timeScale = 1.0f;
		bool paused = false;
		std::uint64_t frameCount = 0;
	};

	namespace TimeSystem
	{
		void Initialize(Time& time,double startTimeSeconds,const TimeConfig& config = TimeConfig{});
		void BeginFrame(Time& time, double currentTimeSeconds);
		bool StepFixed(Time& time);
		void SetTimeScale(Time& time, float scale);
		void SetPaused(Time& time, bool paused);
	}
}