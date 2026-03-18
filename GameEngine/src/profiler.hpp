#pragma once

#include "ring_buffer.hpp"

constexpr size_t PROFILER_CAP = 1024;

/*
  STRUCTURE :
	RingBuffer of frames
	{
		FrameData
		{
			->  frame time
			->  samples
				{
					ProfileSample
				}
		}
	}
*/

constexpr size_t MAX_SAMPLES_PER_FRAME = 256;
constexpr size_t MAX_SCOPE_DEPTH = 32;

struct ProfileSampleNode
{
	const char* name{ nullptr };
	float       startMs{ 0.f };
	float       durationMs{ 0.f };

	std::vector<ProfileSampleNode> children{};
};

struct ProfileSample
{
	const char* name{ nullptr };
	float startMs{ 0.f };
	float durationMs{ 0.f };
	size_t depth{ 0 }; //stack depth for hierarchy view
};

struct FrameData
{
	//std::array<ProfileSample,MAX_SAMPLES_PER_FRAME> samples;
	std::vector<ProfileSampleNode> roots;
	float frameTimeMs = 0.0f;
};

class Profiler
{
	using Frames = RingBuffer<FrameData, PROFILER_CAP>;
	using Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<Clock>;

private:
	FrameData _displayFrame{};
	FrameData _currentFrame{};
	Frames _frames{};

	std::array<ProfileSampleNode*, MAX_SCOPE_DEPTH> _scopeStack{};
	size_t _scopeDepth;

	bool _paused;
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

	const FrameData& GetDisplayFrame() const { return _displayFrame; }
	const Frames& GetFrames() const	{ return _frames; }
	void SetPaused(bool pause)
	{
		_paused = pause;
	}

	const bool IsPaused() { return _paused; }
	Profiler() : _scopeDepth(0), _paused(false) {};

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