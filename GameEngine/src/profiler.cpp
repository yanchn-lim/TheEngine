#include "profiler.hpp"
#include <iostream>

void Profiler::BeginFrame()
{
	if (_paused) return;
	_frameStart = Clock::now();
	_scopeDepth = 0; //reset depth

	_currentFrame = {}; //reset frame
	_currentFrame.roots.reserve(16); //reserve some roots
}

void Profiler::EndFrame()
{
	if (_paused) return;

	//get time
	auto frameMs = ToMs(Clock::now() - _frameStart);

	_currentFrame.frameTimeMs = frameMs;
	_displayFrame = _currentFrame;
	_frames.Push(_currentFrame);
}

void Profiler::PushScope(const char* name)
{
	if (_paused) return;

	if (_scopeDepth >= MAX_SCOPE_DEPTH) return;

	ProfileSampleNode node;
	node.name = name;
	node.startMs = ToMs(Clock::now() - _frameStart);

	ProfileSampleNode* inserted = nullptr;

	//create an open scope
	if (_scopeDepth == 0) //root node
	{
		_currentFrame.roots.push_back(std::move(node));
		inserted = &_currentFrame.roots.back();
	}
	else
	{
		//parent should exist -1 depth
		ProfileSampleNode* parent = _scopeStack[_scopeDepth - 1];
		parent->children.push_back(std::move(node));
		inserted = &parent->children.back();
	}

	_scopeStack[_scopeDepth] = inserted;
	++_scopeDepth;
}

void Profiler::PopScope()
{
	if (_paused) return;
	if (_scopeDepth == 0) return;

	--_scopeDepth;
	ProfileSampleNode* node = _scopeStack[_scopeDepth];
	//update timing
	node->durationMs = ToMs(Clock::now() - _frameStart) - node->startMs;
}
