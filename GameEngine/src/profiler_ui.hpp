#pragma once

#include "profiler.hpp"

class ProfilerUI
{
public:
	void Draw();

private:
	void DrawFrameGraph();
	void DrawScopeTable();
	void DrawNode(const ProfileSampleNode& node, float parentMs);

	bool _open = true;
};