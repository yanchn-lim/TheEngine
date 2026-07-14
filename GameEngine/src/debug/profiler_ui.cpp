#include "profiler_ui.hpp"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "memory_tracker.hpp"
#include "imgui.h"

namespace
{
	constexpr size_t ANALYSIS_FRAME_COUNT = 120;
	constexpr float FRAME_BUDGET_MS = 16.66f;

	struct FrameSummary
	{
		float currentMs{};
		float averageMs{};
		float maxMs{};
		float p95Ms{};
		size_t count{};
	};

	struct ScopeAggregate
	{
		std::string name;
		double totalMs{};
		float maxMs{};
		size_t count{};
	};

	const char* FormatBytes(size_t bytes, char* buffer, size_t bufferSize)
	{
		constexpr const char* units[] = { "B", "KB", "MB", "GB" };
		double value = static_cast<double>(bytes);
		size_t unit = 0;
		while (value >= 1024.0 && unit < 3)
		{
			value /= 1024.0;
			++unit;
		}

		snprintf(buffer, bufferSize, "%.2f %s", value, units[unit]);
		return buffer;
	}

	size_t GetHistoryStart(const RingBuffer<FrameData, PROFILER_CAP>& frames)
	{
		return (frames.head + PROFILER_CAP - frames.count) % PROFILER_CAP;
	}

	FrameSummary CalculateFrameSummary(const RingBuffer<FrameData, PROFILER_CAP>& frames)
	{
		FrameSummary summary;
		summary.count = std::min(frames.count, ANALYSIS_FRAME_COUNT);
		if (summary.count == 0)
			return summary;

		std::array<float, ANALYSIS_FRAME_COUNT> values{};
		const size_t start = (frames.head + PROFILER_CAP - summary.count) % PROFILER_CAP;
		for (size_t i = 0; i < summary.count; ++i)
		{
			const float value = frames.data[(start + i) % PROFILER_CAP].frameTimeMs;
			values[i] = value;
			summary.averageMs += value;
			summary.maxMs = std::max(summary.maxMs, value);
		}

		summary.currentMs = values[summary.count - 1];
		summary.averageMs /= static_cast<float>(summary.count);
		std::sort(values.begin(), values.begin() + summary.count);
		const size_t p95Index = static_cast<size_t>(std::ceil(summary.count * 0.95f)) - 1;
		summary.p95Ms = values[p95Index];
		return summary;
	}

	void AccumulateScope(const ProfileSampleNode& node, std::unordered_map<std::string, ScopeAggregate>& aggregates)
	{
		const char* name = node.name ? node.name : "Unnamed";
		ScopeAggregate& aggregate = aggregates[name];
		aggregate.name = name;
		aggregate.totalMs += node.durationMs;
		aggregate.maxMs = std::max(aggregate.maxMs, node.durationMs);
		++aggregate.count;

		for (const ProfileSampleNode& child : node.children)
			AccumulateScope(child, aggregates);
	}

}

void ProfilerUI::Draw()
{
	if (!_open)
		return;

	ImGui::SetNextWindowSize(ImVec2(1000.f, 760.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(820.f, 560.f), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Profiler", &_open);

	const FrameData& displayFrame = Profiler::Get().GetDisplayFrame();
	DrawHeader(displayFrame);
	DrawFrameHistory();
	ImGui::Separator();

	const float upperHeight = std::max(260.f, ImGui::GetContentRegionAvail().y * 0.54f);
	if (ImGui::BeginTable("ProfilerUpper", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(0.f, upperHeight)))
	{
		ImGui::TableSetupColumn("Timeline", ImGuiTableColumnFlags_WidthStretch, 1.45f);
		ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthStretch, 1.f);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		const FrameData& selectedFrame = _followLatest ? displayFrame : _pinnedFrame;
		DrawScopeTimeline(selectedFrame);

		ImGui::TableSetColumnIndex(1);
		DrawMemoryPanel();

		ImGui::EndTable();
	}

	ImGui::Separator();
	DrawRecentScopeCost();
	ImGui::End();
}

bool& ProfilerUI::IsOpen()
{
	return _open;
}

void ProfilerUI::DrawHeader(const FrameData& displayFrame)
{
	const FrameSummary summary = CalculateFrameSummary(Profiler::Get().GetFrames());
	const float fps = displayFrame.frameTimeMs > 0.f ? 1000.f / displayFrame.frameTimeMs : 0.f;
	ImGui::Text("Frame %.2f ms | %.1f FPS | Avg %.2f ms | P95 %.2f ms | Max %.2f ms (%zu frames)",
		displayFrame.frameTimeMs, fps, summary.averageMs, summary.p95Ms, summary.maxMs, summary.count);

	ImGui::SameLine();
	if (ImGui::Button(Profiler::Get().IsPaused() ? "Resume" : "Pause"))
		Profiler::Get().SetPaused(!Profiler::Get().IsPaused());

	if (!_followLatest)
	{
		ImGui::SameLine();
		if (ImGui::Button("Follow Latest"))
		{
			_followLatest = true;
			_selectedScope = {};
		}
	}
}

void ProfilerUI::DrawFrameHistory()
{
	const auto& frames = Profiler::Get().GetFrames();
	if (frames.count == 0)
	{
		ImGui::TextDisabled("Waiting for frame history...");
		return;
	}

	if (!_historyHeadInitialized)
	{
		_lastHistoryHead = frames.head;
		_historyHeadInitialized = true;
	}
	else if (frames.head != _lastHistoryHead)
	{
		if (!_followLatest)
			_pinnedFrameAge += (frames.head + PROFILER_CAP - _lastHistoryHead) % PROFILER_CAP;
		_lastHistoryHead = frames.head;
	}

	std::array<float, PROFILER_CAP> history{};
	const size_t historyStart = GetHistoryStart(frames);
	for (size_t i = 0; i < frames.count; ++i)
		history[i] = frames.data[(historyStart + i) % PROFILER_CAP].frameTimeMs;

	constexpr float plotHeight = 88.f;
	const ImVec2 plotPosition = ImGui::GetCursorScreenPos();
	const ImVec2 plotSize = ImVec2(ImGui::GetContentRegionAvail().x, plotHeight);
	ImGui::PlotLines("##FrameHistory", history.data(), static_cast<int>(frames.count), 0, nullptr, 0.f, 50.f, plotSize);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float budgetY = plotPosition.y + plotSize.y * (1.f - FRAME_BUDGET_MS / 50.f);
	drawList->AddLine(ImVec2(plotPosition.x, budgetY), ImVec2(plotPosition.x + plotSize.x, budgetY), IM_COL32(255, 126, 104, 170));
	drawList->AddText(ImVec2(plotPosition.x + plotSize.x - 48.f, budgetY - 14.f), IM_COL32(255, 126, 104, 190), "16.6 ms");

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const float normalizedX = std::clamp((ImGui::GetIO().MousePos.x - plotPosition.x) / plotSize.x, 0.f, 0.9999f);
		const size_t sampleIndex = std::min(static_cast<size_t>(normalizedX * frames.count), frames.count - 1);
		_pinnedFrame = frames.data[(historyStart + sampleIndex) % PROFILER_CAP];
		_pinnedFrameAge = frames.count - 1 - sampleIndex;
		_followLatest = false;
		_selectedScope = {};
	}

	size_t markerIndex = frames.count - 1;
	if (!_followLatest)
	{
		if (_pinnedFrameAge >= frames.count)
		{
			ImGui::TextDisabled("Pinned frame is outside the retained history.");
			return;
		}
		markerIndex = frames.count - 1 - _pinnedFrameAge;
	}

	const float markerX = plotPosition.x + plotSize.x * (static_cast<float>(markerIndex) / std::max<size_t>(frames.count - 1, 1));
	drawList->AddLine(ImVec2(markerX, plotPosition.y), ImVec2(markerX, plotPosition.y + plotSize.y), IM_COL32(111, 210, 188, 220), 2.f);
	ImGui::TextDisabled(_followLatest ? "Following latest frame. Click history to inspect a captured frame." : "Pinned frame. Follow Latest resumes live inspection.");
}

void ProfilerUI::DrawScopeTimeline(const FrameData& frame)
{
	ImGui::BeginChild("ScopeTimelinePane", ImVec2(0.f, 0.f), true);
	ImGui::TextUnformatted("Selected Frame Breakdown");
	ImGui::SameLine();
	ImGui::TextDisabled("%.2f ms", frame.frameTimeMs);

	if (frame.roots.empty() || frame.frameTimeMs <= 0.f)
	{
		ImGui::TextDisabled("No scope data in the selected frame.");
		ImGui::EndChild();
		return;
	}

	const float inspectorHeight = 46.f;
	const float tableHeight = std::max(80.f, ImGui::GetContentRegionAvail().y - inspectorHeight);
	constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
	if (ImGui::BeginTable("ScopeBreakdown", 4, tableFlags, ImVec2(0.f, tableHeight)))
	{
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 70.f);
		ImGui::TableSetupColumn("Frame %", ImGuiTableColumnFlags_WidthFixed, 64.f);
		ImGui::TableSetupColumn("Parent %", ImGuiTableColumnFlags_WidthFixed, 68.f);
		ImGui::TableHeadersRow();

		for (const ProfileSampleNode& root : frame.roots)
			DrawScopeNode(root, frame.frameTimeMs, frame.frameTimeMs);

		ImGui::EndTable();
	}

	if (_selectedScope.valid)
	{
		const float parentShare = _selectedScope.parentDurationMs > 0.f ? (_selectedScope.durationMs / _selectedScope.parentDurationMs) * 100.f : 0.f;
		const float frameShare = _selectedScope.frameDurationMs > 0.f ? (_selectedScope.durationMs / _selectedScope.frameDurationMs) * 100.f : 0.f;
		ImGui::Text("%s | %.3f ms at %.3f ms | %.1f%% parent | %.1f%% frame",
			_selectedScope.name.c_str(), _selectedScope.durationMs, _selectedScope.startMs, parentShare, frameShare);
	}
	else
	{
		ImGui::TextDisabled("Select a scope row to inspect its cost.");
	}

	ImGui::EndChild();
}

void ProfilerUI::DrawScopeNode(const ProfileSampleNode& node, float parentDurationMs, float frameDurationMs)
{
	const bool selected = _selectedScope.valid
		&& _selectedScope.name == (node.name ? node.name : "Unnamed")
		&& _selectedScope.startMs == node.startMs;
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;
	if (node.children.empty())
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (selected)
		flags |= ImGuiTreeNodeFlags_Selected;

	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	const bool open = ImGui::TreeNodeEx(node.name ? node.name : "Unnamed", flags);
	const bool clicked = ImGui::IsItemClicked();

	ImGui::TableSetColumnIndex(1);
	ImGui::Text("%.3f ms", node.durationMs);
	ImGui::TableSetColumnIndex(2);
	ImGui::Text("%.1f%%", (node.durationMs / frameDurationMs) * 100.f);
	ImGui::TableSetColumnIndex(3);
	const float parentShare = parentDurationMs > 0.f ? (node.durationMs / parentDurationMs) * 100.f : 0.f;
	ImGui::Text("%.1f%%", parentShare);

	if (clicked)
	{
		_selectedScope.name = node.name ? node.name : "Unnamed";
		_selectedScope.startMs = node.startMs;
		_selectedScope.durationMs = node.durationMs;
		_selectedScope.parentDurationMs = parentDurationMs;
		_selectedScope.frameDurationMs = frameDurationMs;
		_selectedScope.valid = true;
	}

	if (open && !node.children.empty())
	{
		for (const ProfileSampleNode& child : node.children)
			DrawScopeNode(child, node.durationMs, frameDurationMs);
		ImGui::TreePop();
	}
}

void ProfilerUI::DrawMemoryPanel()
{
	ImGui::BeginChild("MemoryPane", ImVec2(0.f, 0.f), true);
	ImGui::TextUnformatted("Memory");

	const Memory::CpuMemoryStats cpu = Memory::GetLastFrameCpuStats();
	_memoryHistory.Push(static_cast<float>(cpu.liveBytes) / (1024.f * 1024.f));

	char liveBytes[32];
	char peakBytes[32];
	char allocatedBytes[32];
	char freedBytes[32];
	ImGui::TextDisabled("CPU HEAP");
	ImGui::Text("%s live | %s peak", FormatBytes(cpu.liveBytes, liveBytes, sizeof(liveBytes)), FormatBytes(cpu.peakLiveBytes, peakBytes, sizeof(peakBytes)));
	ImGui::TextDisabled("%zu live allocations", cpu.liveAllocationCount);
	ImGui::TextColored(ImVec4(0.88f, 0.68f, 0.30f, 1.f), "+ %s", FormatBytes(cpu.frameAllocatedBytes, allocatedBytes, sizeof(allocatedBytes)));
	ImGui::SameLine();
	ImGui::TextDisabled("(%zu allocs)", cpu.frameAllocationCount);
	ImGui::TextColored(ImVec4(0.35f, 0.78f, 0.62f, 1.f), "- %s", FormatBytes(cpu.frameFreedBytes, freedBytes, sizeof(freedBytes)));
	ImGui::SameLine();
	ImGui::TextDisabled("(%zu frees)", cpu.frameFreeCount);

	float historyMax = 1.f;
	for (size_t i = 0; i < _memoryHistory.count; ++i)
		historyMax = std::max(historyMax, _memoryHistory[i]);
	ImGui::TextDisabled("CPU LIVE MEMORY (MB)");
	ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.35f, 0.78f, 0.62f, 1.f));
	ImGui::PlotLines("##CpuMemoryHistory", _memoryHistory.data.data(), static_cast<int>(_memoryHistory.count), static_cast<int>(_memoryHistory.head), nullptr, 0.f, historyMax * 1.1f, ImVec2(-1.f, 56.f));
	ImGui::PopStyleColor();

	auto resources = Memory::GetResourceMemoryStats();
	std::sort(resources.begin(), resources.end(), [](const Memory::ResourceMemoryStats& left, const Memory::ResourceMemoryStats& right)
		{
			return left.currentBytes > right.currentBytes;
		});

	ImGui::TextDisabled("GPU ESTIMATED RESOURCES");
	if (resources.empty())
	{
		ImGui::TextDisabled("No tracked GPU resources.");
		ImGui::EndChild();
		return;
	}

	constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
	if (ImGui::BeginTable("MemoryResources", 4, tableFlags, ImVec2(0.f, 0.f)))
	{
		ImGui::TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 78.f);
		ImGui::TableSetupColumn("Peak", ImGuiTableColumnFlags_WidthFixed, 78.f);
		ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 46.f);
		ImGui::TableHeadersRow();

		for (const Memory::ResourceMemoryStats& resource : resources)
		{
			char currentBytes[32];
			char peakBytes[32];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(resource.label.c_str());
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", resource.label.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(FormatBytes(resource.currentBytes, currentBytes, sizeof(currentBytes)));
			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(FormatBytes(resource.peakBytes, peakBytes, sizeof(peakBytes)));
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%zu", resource.resourceCount);
		}

		ImGui::EndTable();
	}

	ImGui::EndChild();
}

void ProfilerUI::DrawRecentScopeCost()
{
	ImGui::TextUnformatted("Recent Scope Cost");
	ImGui::SameLine();
	ImGui::TextDisabled("Last %zu frames", ANALYSIS_FRAME_COUNT);

	const auto& frames = Profiler::Get().GetFrames();
	const size_t frameCount = std::min(frames.count, ANALYSIS_FRAME_COUNT);
	if (frameCount == 0)
	{
		ImGui::TextDisabled("No captured frames.");
		return;
	}

	std::unordered_map<std::string, ScopeAggregate> aggregateMap;
	double totalFrameMs = 0.0;
	const size_t start = (frames.head + PROFILER_CAP - frameCount) % PROFILER_CAP;
	for (size_t i = 0; i < frameCount; ++i)
	{
		const FrameData& frame = frames.data[(start + i) % PROFILER_CAP];
		totalFrameMs += frame.frameTimeMs;
		for (const ProfileSampleNode& root : frame.roots)
			AccumulateScope(root, aggregateMap);
	}

	std::vector<ScopeAggregate> aggregates;
	aggregates.reserve(aggregateMap.size());
	for (const auto& [name, aggregate] : aggregateMap)
		aggregates.push_back(aggregate);
	std::sort(aggregates.begin(), aggregates.end(), [](const ScopeAggregate& left, const ScopeAggregate& right)
		{
			return (left.totalMs / left.count) > (right.totalMs / right.count);
		});

	constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable;
	if (!ImGui::BeginTable("RecentScopeCost", 6, tableFlags, ImVec2(0.f, 0.f)))
		return;

	ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthFixed, 72.f, 1);
	ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 72.f, 2);
	ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 78.f, 3);
	ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 58.f, 4);
	ImGui::TableSetupColumn("Frame %", ImGuiTableColumnFlags_WidthFixed, 70.f, 5);
	ImGui::TableHeadersRow();

	if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); sortSpecs && sortSpecs->SpecsCount > 0)
	{
		const ImGuiTableColumnSortSpecs& sort = sortSpecs->Specs[0];
		std::sort(aggregates.begin(), aggregates.end(), [&](const ScopeAggregate& left, const ScopeAggregate& right)
			{
				const double leftAverage = left.totalMs / left.count;
				const double rightAverage = right.totalMs / right.count;
				int comparison = 0;
				switch (sort.ColumnUserID)
				{
				case 0: comparison = left.name.compare(right.name); break;
				case 1: comparison = (leftAverage > rightAverage) - (leftAverage < rightAverage); break;
				case 2: comparison = (left.maxMs > right.maxMs) - (left.maxMs < right.maxMs); break;
				case 3: comparison = (left.totalMs > right.totalMs) - (left.totalMs < right.totalMs); break;
				case 4: comparison = (left.count > right.count) - (left.count < right.count); break;
				case 5: comparison = (left.totalMs > right.totalMs) - (left.totalMs < right.totalMs); break;
				default: break;
				}
				if (comparison == 0)
					comparison = left.name.compare(right.name);
				return sort.SortDirection == ImGuiSortDirection_Ascending ? comparison < 0 : comparison > 0;
			});
			sortSpecs->SpecsDirty = false;
	}

	for (const ScopeAggregate& aggregate : aggregates)
	{
		const double average = aggregate.totalMs / aggregate.count;
		const double frameShare = totalFrameMs > 0.0 ? (aggregate.totalMs / totalFrameMs) * 100.0 : 0.0;
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(aggregate.name.c_str());
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f ms", average);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f ms", aggregate.maxMs);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.2f ms", aggregate.totalMs);
		ImGui::TableSetColumnIndex(4);
		ImGui::Text("%zu", aggregate.count);
		ImGui::TableSetColumnIndex(5);
		ImGui::Text("%.1f%%", frameShare);
	}

	ImGui::EndTable();
}
