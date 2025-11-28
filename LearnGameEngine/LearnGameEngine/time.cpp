#include "time.hpp"
#include <GLFW/glfw3.h>

namespace Engine
{
    namespace
    {
        double s_LastTimeSeconds = 0.0;
        float s_MaxDeltaTime = 0.25f;
    }

    namespace TimeSystem
    {
        static float lastFrameTime = 0.0f;

        void Initialize(Time& time, double startTimeSeconds,const TimeConfig& config)
        {
            //set values
            s_LastTimeSeconds = startTimeSeconds;
            s_MaxDeltaTime = config.kMaxDeltaTime;

            time.deltaTime = 0.0f;
            time.unscaledDeltaTime = 0.0f;

            time.fixedDeltaTime = config.kFixedDeltaTime;

            time.timeSinceStart = 0.0f;
            time.unscaledTimeSinceStart = 0.0f;

            time.accumulator = 0.0f;

            time.timeScale = config.kInitialTimeScale;
            time.paused = false;

            time.frameCount = 0;
        }

        void BeginFrame(Time& time, double currentTimeSeconds)
        {
            //get delta time
            double rawDT = currentTimeSeconds - s_LastTimeSeconds;
            s_LastTimeSeconds = currentTimeSeconds;

            //prevent negative values
            rawDT = rawDT < 0.0 ? 0 : rawDT;
            //clamp to max dt
            rawDT = rawDT > static_cast<double>(s_MaxDeltaTime) ? static_cast<double>(s_MaxDeltaTime) : rawDT;

            time.unscaledDeltaTime = static_cast<float>(rawDT);
            time.unscaledTimeSinceStart += time.unscaledDeltaTime;

            float scaledDT = time.paused ? 0.0f : time.unscaledDeltaTime * time.timeScale;
            time.deltaTime = scaledDT;
            time.timeSinceStart += scaledDT;

            time.accumulator += scaledDT;

            ++time.frameCount;
        }

        bool StepFixed(Time& time)
        {
            if (time.accumulator >= time.fixedDeltaTime)
            {
                time.accumulator -= time.fixedDeltaTime;
                return true;
            }

            return false;
        }

        void SetTimeScale(Time& time, float scale)
        {
            scale = scale < 0.0f ? 0.0f : scale;

            time.timeScale = scale;
        }

        void SetPaused(Time& time, bool paused)
        {
            time.paused = paused;
        }
    }
}
