// debug_system.hpp
#pragma once

#include "events.hpp"
#include "debug.hpp"

namespace Engine
{
    class DebugFrameListener
    {
    public:
        explicit DebugFrameListener(EventBus& bus)
        {
            bus.Subscribe<FrameEndEvent>(
                [](const FrameEndEvent& e)
                {
                    Debug::LogTimeStats(e.time);
                });
        }
    };
}
