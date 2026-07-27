#pragma once

#include "profiler_ui.hpp"

class DebugOverlay
{
public:
    void HandleKey(int key, int action);
    void Draw();

private:
    ProfilerUI _profilerUI;
};
