#include "debug_overlay.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "debug.hpp"
#include "profiler.hpp"

void DebugOverlay::HandleKey(int key, int action)
{
    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_F5)
        Profiler::Get().SetPaused(!Profiler::Get().IsPaused());
    else if (key == GLFW_KEY_F6)
    {
        Profiler::Get().PrintFrameStatistics(2048);
        Profiler::Get().PrintFrameStatisticsToFile("FRAME_STATS.txt", 2048);
    }
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

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    _profilerUI.Draw();
    DebugConsole::Get().Draw();
}
