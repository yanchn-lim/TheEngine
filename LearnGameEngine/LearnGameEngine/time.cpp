#include "time.hpp"
#include <GLFW/glfw3.h>

namespace Engine
{
    namespace TimeSystem
    {
        static float lastFrameTime = 0.0f;

        void Initialize(Time& time)
        {
            lastFrameTime = glfwGetTime();
            time.timeSinceStart = lastFrameTime;
            time.deltaTime = 0.0f;
            time.accumulator = 0.0f;
            time.fixedDeltaTime = 1.0f / 60.0f;
            time.frameCount = 0;
        }

        void Update(Time& time)
        {
            float now = glfwGetTime();

            // Variable timestep
            time.deltaTime = now - lastFrameTime;
            lastFrameTime = now;

            // Absolute engine time
            time.timeSinceStart = now;

            // Accumulate for fixed timestep
            time.accumulator += time.deltaTime;

            // Frame counter
            time.frameCount++;
        }
    }
}
