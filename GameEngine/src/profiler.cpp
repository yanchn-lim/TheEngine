#include "profiler.hpp"
#include <iostream>

void Profiler::Record(const char* name, float ms)
{
	auto& record = _scopes[name];
	auto& history = record.history;
	float sum{ 0 };
	for (int i = 0; i < history.count; ++i)
	{
		sum += history.data[i];
	}

	record.avgMs = sum / static_cast<float>(history.count);
	record.lastMs = ms;
	record.history.Push(ms);
}