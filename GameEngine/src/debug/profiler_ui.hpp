#pragma once

#include "profiler.hpp"

class ProfilerUI
{
public:
	void Draw();
	bool& IsOpen();

private:
	struct ScopeSelection
	{
		std::string name;
		float startMs{};
		float durationMs{};
		float parentDurationMs{};
		float frameDurationMs{};
		bool valid{};
	};

	void DrawHeader(const FrameData& displayFrame);
	void DrawFrameHistory();
	void DrawScopeTimeline(const FrameData& frame);
	void DrawScopeNode(const ProfileSampleNode& node, float parentDurationMs, float frameDurationMs);
	void DrawRecentScopeCost();
	void DrawMemoryPanel();

	bool _open = true;
	bool _followLatest = true;
	bool _historyHeadInitialized = false;
	size_t _lastHistoryHead{};
	size_t _pinnedFrameAge{};
	FrameData _pinnedFrame;
	ScopeSelection _selectedScope;
	RingBuffer<float, PROFILER_CAP> _memoryHistory;
};
