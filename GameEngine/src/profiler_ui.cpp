#include "profiler.hpp"
#include "profiler_ui.hpp"

#include "imgui.h"

void ProfilerUI::Draw()
{
	if (!_open) return;

	ImGui::SetNextWindowSize(ImVec2(480, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Profiler", &_open);

	DrawFrameGraph();
	//separator
	DrawScopeTable();

	ImGui::End();
}

void ProfilerUI::DrawFrameGraph()
{
	const auto& scopes = Profiler::Get().GetScopes();
	auto it = scopes.find("MainLoop"); //get entry point

	float lastMs = 0.f;
	float avgMs = 0.f;

	//retrieve latest data
	if (it != scopes.end())
	{
		lastMs = it->second.lastMs;
		avgMs = it->second.avgMs;
	}

	float fps = lastMs > 0.0f ? 1000.0f / lastMs : 0.0f;
	float avgFps = avgMs > 0.0f ? 1000.0f / avgMs : 0.0f;
	ImGui::Text("Frame time : %.2f ms   (avg %.2f ms)", lastMs, avgMs);
	ImGui::Text("FPS        : %.1f      (avg %.1f)", fps, avgFps);
}

void ProfilerUI::DrawScopeTable()
{

}
