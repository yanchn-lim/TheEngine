#pragma once

#include "profiler_ui.hpp"

class DebugOverlay
{
public:
	void ToggleProfilerPause();
	void PrintProfilerStatistics();
    void Draw();

private:
    ProfilerUI _profilerUI;
};
