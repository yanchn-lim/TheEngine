#pragma once

#include "ring_buffer.hpp"

constexpr size_t PROFILER_CAP = 1024;

struct ProfileSample
{
	const char* name{ nullptr };
	float startMs{ 0.f };
	float durationMs{ 0.f };
	int depth{ 0 }; //stack depth for hierarchy view
};

struct FrameData
{
	std::vector<ProfileSample> samples;
	float frameTimeMs = 0.0f;
};

struct ScopeData
{
	RingBuffer<float, PROFILER_CAP> history;
	float lastMs = 0.0f;
	float avgMs = 0.0f;
};




class Profiler
{
	using Scopes = std::unordered_map<std::string, ScopeData>;
private:
	Scopes _scopes;

public:
	static Profiler& Get()
	{
		static Profiler instance;
		return instance;
	}
	
	void Record(const char* name,float ms);

	const Scopes& GetScopes() const
	{
		return _scopes;
	}

	Profiler() {};
	//singleton?
	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler&) = delete;
};

//RAII profile scope
struct ProfileScope
{
	const char* name;
	std::chrono::high_resolution_clock::time_point start;

	ProfileScope(const char* name) : name(name), start(std::chrono::high_resolution_clock::now()) {};

	~ProfileScope()
	{
		auto end = std::chrono::high_resolution_clock::now();
		float ms = std::chrono::duration<float, std::milli>(end - start).count();
		Profiler::Get().Record(name, ms);
	}
};

//MACROS
#define PROFILE_SCOPE(name) ProfileScope _profile_scope_##__LINE__(name)
#define PROFILE_FUNCTION()  ProfileScope _profile_scope_##__LINE__(__func__)