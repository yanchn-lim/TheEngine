#include "profiler.hpp"
#include "profiler_ui.hpp"

#include "imgui.h"

void ProfilerUI::Draw()
{
	PROFILE_FUNCTION();
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

	ImGui::PlotLines("Frame Time History", it != scopes.end() ? it->second.history.data.data() : nullptr, 
		it != scopes.end() ? (int)it->second.history.count : 0, 
		0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));


	ImGui::BeginChild("ProfilerHierarchyView", ImVec2(0, 0), true);

	ImGui::BeginTable("dsads", 2);
	//draw tree
	ImGui::TableSetupColumn("Hierarchy");
	ImGui::TableSetupColumn("Time");
	ImGui::TableHeadersRow();
	for (auto& [name, scope] : scopes)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		if (ImGui::TreeNode(name.c_str()))
		{
			
			ImGui::TreePop();
		}
		
		//draw same line with ms and fps
		ImGui::TableSetColumnIndex(1);
		ImGui::Text(" - %.2f ms (avg %.2f ms)", scope.lastMs, scope.avgMs);
	}
	ImGui::EndTable();
	
	ImGui::EndChild();
}

void ProfilerUI::DrawScopeTable()
{

}
