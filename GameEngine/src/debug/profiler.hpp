#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include "core/ring_buffer.hpp"

constexpr size_t PROFILER_CAP = 1024;
constexpr size_t MAX_SCOPE_DEPTH = 32;

struct ProfileSampleNode
{
	const char* name{ nullptr };
	float       startMs{ 0.f };
	float       durationMs{ 0.f };

	std::vector<ProfileSampleNode> children{};
};

struct FrameData
{
	std::vector<ProfileSampleNode> roots;
	float frameTimeMs = 0.0f;
};

class Profiler
{
	using Frames = RingBuffer<FrameData, PROFILER_CAP>;
	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::time_point<Clock>;

private:
	FrameData _emptyFrame{};
	FrameData _currentFrame{};
	Frames _frames{};

	std::array<ProfileSampleNode*, MAX_SCOPE_DEPTH> _scopeStack{};
	size_t _scopeDepth;
	size_t _ignoredScopeDepth;

	bool _paused;
	bool _pauseRequested;
	bool _capturing;
	TimePoint _frameStart;

	double ToMs(std::chrono::duration<double> d)
	{
		return d.count() * 1000.0;
	}
public:
	static Profiler& Get()
	{
		static Profiler instance;
		return instance;
	}
	
	void BeginFrame();
	void EndFrame();

	void PushScope(const char* name);
	void PopScope();
	void PrintFrameStatistics(size_t numFrames = 100) const;
	void PrintFrameStatisticsToFile(const std::string& filename, size_t numFrames = 100) const;

	const FrameData& GetDisplayFrame() const
	{
		return _frames.count > 0
			? _frames.data[(_frames.head + PROFILER_CAP - 1) % PROFILER_CAP]
			: _emptyFrame;
	}
	const Frames& GetFrames() const	{ return _frames; }
	void RequestPaused(bool pause) { _pauseRequested = pause; }

	bool IsPaused() const { return _pauseRequested; }
	Profiler()
		: _scopeDepth(0),
		_ignoredScopeDepth(0),
		_paused(false),
		_pauseRequested(false),
		_capturing(false)
	{
	}

	//singleton?
	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler&) = delete;
};

//RAII profile scope
struct ProfileScope
{
	//on construct, pushes the scope into profiler
	ProfileScope(const char* name)
	{
		Profiler::Get().PushScope(name);
	};

	//on destruct, pop the scope and record
	~ProfileScope()
	{
		Profiler::Get().PopScope();
	}
};

//MACROS
#define PROFILE_SCOPE(name) ProfileScope _profile_scope_##__LINE__(name)
#define PROFILE_FUNCTION()  ProfileScope _profile_scope_##__LINE__(__func__)
