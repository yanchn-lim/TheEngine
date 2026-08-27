#include "debug_overlay.hpp"

#include <imgui.h>

#include "debug.hpp"
#include "profiler.hpp"

void DebugOverlay::ToggleProfilerPause()
{
	Profiler::Get().RequestPaused(!Profiler::Get().IsPaused());
}

void DebugOverlay::PrintProfilerStatistics()
{
	Profiler::Get().PrintFrameStatistics(2048);
	Profiler::Get().PrintFrameStatisticsToFile("FRAME_STATS.txt", 2048);
}

void DebugOverlay::Draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Profiler", nullptr, &_profilerUI.IsOpen());
            ImGui::MenuItem("Console", nullptr, &DebugConsole::Get().IsOpen());
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    _profilerUI.Draw();
    DebugConsole::Get().Draw();
}
