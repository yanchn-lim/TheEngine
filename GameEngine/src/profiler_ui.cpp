#include "profiler_ui.hpp"

#include "imgui.h"

void ProfilerUI::DrawNode(const ProfileSampleNode& node, float parentMs)
{
	float pct = parentMs > 0.f ? (node.durationMs / parentMs) * 100.f : 0.f;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth
		| ImGuiTreeNodeFlags_DefaultOpen;
	if (node.children.empty())
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	bool open = ImGui::TreeNodeEx(node.name, flags);

	ImGui::TableSetColumnIndex(1);
	ImGui::Text("%.3f ms", node.durationMs);
	ImGui::TableSetColumnIndex(2);
	ImGui::Text("%.1f%%", pct);

	if (open && !node.children.empty())
	{
		for (const ProfileSampleNode& child : node.children)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			DrawNode(child, node.durationMs);
		}
		ImGui::TreePop();
	}
}

void ProfilerUI::Draw()
{
	//PROFILE_FUNCTION();
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
	const FrameData& display = Profiler::Get().GetDisplayFrame();

	if (display.roots.empty())
	{
		ImGui::Text("No data yet...");
		return;
	}

	const auto& frames = Profiler::Get().GetFrames();

	//build timing history
	static std::array<float, PROFILER_CAP> history;
	for (size_t i = 0; i < PROFILER_CAP; ++i)
		history[i] = frames[i].frameTimeMs;

	float lastMs = display.frameTimeMs;
	float fps = lastMs > 0.f ? 1000.f / lastMs : 0.f;

	ImGui::Text("Frame: %.2f ms  |  FPS: %.1f", lastMs, fps);

	constexpr float plotHeight = 60.f;
	constexpr float scaleMin = 0.f;
	constexpr float scaleMax = 50.f;
	constexpr float targetMs = 16.66f;

	ImVec2 plotPos = ImGui::GetCursorScreenPos();
	ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, plotHeight);

	//draw graph
	ImGui::PlotLines("##FrameTime", history.data(), (int)PROFILER_CAP, (int)frames.head, nullptr, scaleMin,scaleMax,plotSize);

	float t = (targetMs - scaleMin) / (scaleMax - scaleMin);         // normalize to [0,1]
	float y = plotPos.y + plotSize.y * (1.f - t);                    // flip — y=0 is top

	//draw 16.66ms target line
	ImDrawList* dl = ImGui::GetWindowDrawList();
	dl->AddLine(
		ImVec2(plotPos.x, y),
		ImVec2(plotPos.x + plotSize.x, y),
		IM_COL32(255, 120, 120, 150),   // red, slightly transparent
		1.f
	);

	dl->AddText(
		ImVec2(plotPos.x + plotSize.x - 48.f, y - 13.f),
		IM_COL32(255, 120, 120, 150),
		"16.6ms"
	);
}

void ProfilerUI::DrawScopeTable()
{
	const FrameData& frame = Profiler::Get().GetDisplayFrame();

	if (frame.roots.empty()) return;

	//rework this later on
	constexpr ImGuiTableFlags tableFlags =
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable;

	if (!ImGui::BeginTable("ScopeTable", 3, tableFlags)) return;

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.f);
	ImGui::TableSetupColumn("% Parent", ImGuiTableColumnFlags_WidthFixed, 70.f);
	ImGui::TableHeadersRow();

	for (const ProfileSampleNode& root : frame.roots)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		DrawNode(root, frame.frameTimeMs);
	}

	ImGui::EndTable();
}
