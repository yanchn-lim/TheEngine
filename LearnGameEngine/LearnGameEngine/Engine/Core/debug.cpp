#include "debug.hpp"
#include <cstdio>

#if defined(_DEBUG)
namespace Engine
{
	namespace Debug
	{
		void LogTimeStats(const Time& time)
		{
			static float timeSinceLastLog = 0.0f;
			timeSinceLastLog += time.unscaledDeltaTime;

			//print per second
			if (timeSinceLastLog >= 1.0f)
			{
				float fps = (time.deltaTime > 0.0f) ? 1.0f / time.deltaTime : 0.0f;

				std::printf(
					"[Time] frame=%llu | t=%.2f | dt=%.4f | "
                    "fixedDt=%.4f | acc=%.4f | fps=%.1f | "
					"scale=%.2f | paused=%d\n", 
					static_cast<unsigned long long>(time.frameCount),
					time.timeSinceStart,
					time.deltaTime,
					time.fixedDeltaTime,
					time.accumulator,
					fps,
					time.timeScale,
					time.paused ? 1 : 0);


				timeSinceLastLog = 0.0f;
			}
		}
		
		void LogSystem()
		{

		}
		/*void Logerror*/
	}
}
#endif
