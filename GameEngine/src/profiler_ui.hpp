#pragma once


class ProfilerUI
{
public:
	void Draw();

private:
	void DrawFrameGraph();
	void DrawScopeTable();

	bool _open = true;
};