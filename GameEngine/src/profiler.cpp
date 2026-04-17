#include "profiler.hpp"

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

void Profiler::PrintFrameStatistics(size_t numFrames) const
{
	size_t available = _frames.count;
	if (available == 0)
	{
		std::cout << "No frames recorded yet.\n";
		return;
	}

	numFrames = std::min(numFrames, available);
	size_t startOffset = available - numFrames;

	std::vector<double> frameTimes;
	frameTimes.reserve(numFrames);

	struct ScopeStats
	{
		size_t count = 0;
		double sum = 0.0;
		double sumSq = 0.0;
		float min = std::numeric_limits<float>::max();
		float max = std::numeric_limits<float>::lowest();
	};
	std::unordered_map<std::string, ScopeStats> scopeStatsMap;

	auto updateStats = [&](const ProfileSampleNode& node, auto&& selfRef) -> void
		{
			float dur = node.durationMs;
			std::string name(node.name ? node.name : "unnamed");

			auto& stats = scopeStatsMap[name];
			stats.count++;
			stats.sum += dur;
			stats.sumSq += static_cast<double>(dur) * dur;
			if (dur < stats.min) stats.min = dur;
			if (dur > stats.max) stats.max = dur;

			for (const auto& child : node.children)
				selfRef(child, selfRef);
		};

	for (size_t i = startOffset; i < available; ++i)
	{
		size_t idx = (_frames.head + i) % PROFILER_CAP;
		const FrameData& frame = _frames.data[idx];

		frameTimes.push_back(frame.frameTimeMs);

		for (const auto& root : frame.roots)
			updateStats(root, updateStats);
	}

	double frameSum = 0.0, frameSumSq = 0.0;
	float frameMin = std::numeric_limits<float>::max();
	float frameMax = std::numeric_limits<float>::lowest();
	for (double t : frameTimes)
	{
		frameSum += t;
		frameSumSq += t * t;
		if (t < frameMin) frameMin = static_cast<float>(t);
		if (t > frameMax) frameMax = static_cast<float>(t);
	}
	double frameMean = frameSum / numFrames;
	double frameVariance = (frameSumSq - frameSum * frameSum / numFrames) / numFrames;
	double frameStdDev = std::sqrt(frameVariance);

	std::cout << std::fixed << std::setprecision(2);
	std::cout << "\n=== Frame Statistics (last " << numFrames << " frames) ===\n";
	std::cout << "Count:  " << numFrames << '\n';
	std::cout << "Mean:   " << frameMean << " ms\n";
	std::cout << "Min:    " << frameMin << " ms\n";
	std::cout << "Max:    " << frameMax << " ms\n";
	std::cout << "StdDev: " << frameStdDev << " ms\n\n";

	if (scopeStatsMap.empty())
	{
		std::cout << "No scope data available.\n";
		return;
	}

	std::vector<std::pair<std::string, ScopeStats>> sortedStats(scopeStatsMap.begin(), scopeStatsMap.end());
	std::sort(sortedStats.begin(), sortedStats.end(),
		[](const auto& a, const auto& b) {
			double meanA = a.second.sum / a.second.count;
			double meanB = b.second.sum / b.second.count;
			return meanA > meanB;
		});

	std::cout << "=== Per-Scope Statistics (durations in ms) ===\n";
	std::cout << std::left << std::setw(30) << "Scope Name"
		<< std::right
		<< std::setw(8) << "Count"
		<< std::setw(10) << "Mean"
		<< std::setw(10) << "Min"
		<< std::setw(10) << "Max"
		<< std::setw(10) << "StdDev"
		<< '\n';
	std::cout << std::string(78, '-') << '\n';

	for (const auto& [name, stats] : sortedStats)
	{
		double mean = stats.sum / stats.count;
		double variance = (stats.sumSq - stats.sum * stats.sum / stats.count) / stats.count;
		double stdDev = std::sqrt(variance);

		std::cout << std::left << std::setw(30) << name.substr(0, 29)
			<< std::right
			<< std::setw(8) << stats.count
			<< std::setw(10) << mean
			<< std::setw(10) << stats.min
			<< std::setw(10) << stats.max
			<< std::setw(10) << stdDev
			<< '\n';
	}
	std::cout << std::endl;
}

void Profiler::PrintFrameStatisticsToFile(const std::string& filename, size_t numFrames) const
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Failed to open file: " << filename << std::endl;
		return;
	}

	size_t available = _frames.count;
	if (available == 0)
	{
		file << "No frames recorded yet.\n";
		return;
	}

	numFrames = std::min(numFrames, available);
	size_t startOffset = available - numFrames;

	std::vector<double> frameTimes;
	frameTimes.reserve(numFrames);

	struct ScopeStats
	{
		size_t count = 0;
		double sum = 0.0;
		double sumSq = 0.0;
		float min = std::numeric_limits<float>::max();
		float max = std::numeric_limits<float>::lowest();
	};
	std::unordered_map<std::string, ScopeStats> scopeStatsMap;

	auto updateStats = [&](const ProfileSampleNode& node, auto&& selfRef) -> void
		{
			float dur = node.durationMs;
			std::string name(node.name ? node.name : "unnamed");

			auto& stats = scopeStatsMap[name];
			stats.count++;
			stats.sum += dur;
			stats.sumSq += static_cast<double>(dur) * dur;
			if (dur < stats.min) stats.min = dur;
			if (dur > stats.max) stats.max = dur;

			for (const auto& child : node.children)
				selfRef(child, selfRef);
		};

	for (size_t i = startOffset; i < available; ++i)
	{
		size_t idx = (_frames.head + i) % PROFILER_CAP;
		const FrameData& frame = _frames.data[idx];
		frameTimes.push_back(frame.frameTimeMs);
		for (const auto& root : frame.roots)
			updateStats(root, updateStats);
	}

	double frameSum = 0.0, frameSumSq = 0.0;
	float frameMin = std::numeric_limits<float>::max();
	float frameMax = std::numeric_limits<float>::lowest();
	for (double t : frameTimes)
	{
		frameSum += t;
		frameSumSq += t * t;
		if (t < frameMin) frameMin = static_cast<float>(t);
		if (t > frameMax) frameMax = static_cast<float>(t);
	}
	double frameMean = frameSum / numFrames;
	double frameVariance = (frameSumSq - frameSum * frameSum / numFrames) / numFrames;
	double frameStdDev = std::sqrt(frameVariance);

	file << std::fixed << std::setprecision(2);
	file << "\n=== Frame Statistics (last " << numFrames << " frames) ===\n";
	file << "Count:  " << numFrames << '\n';
	file << "Mean:   " << frameMean << " ms\n";
	file << "Min:    " << frameMin << " ms\n";
	file << "Max:    " << frameMax << " ms\n";
	file << "StdDev: " << frameStdDev << " ms\n\n";

	if (scopeStatsMap.empty())
	{
		file << "No scope data available.\n";
		return;
	}

	std::vector<std::pair<std::string, ScopeStats>> sortedStats(scopeStatsMap.begin(), scopeStatsMap.end());
	std::sort(sortedStats.begin(), sortedStats.end(),
		[](const auto& a, const auto& b) {
			double meanA = a.second.sum / a.second.count;
			double meanB = b.second.sum / b.second.count;
			return meanA > meanB;
		});

	file << "=== Per-Scope Statistics (durations in ms) ===\n";
	file << std::left << std::setw(30) << "Scope Name"
		<< std::right
		<< std::setw(8) << "Count"
		<< std::setw(10) << "Mean"
		<< std::setw(10) << "Min"
		<< std::setw(10) << "Max"
		<< std::setw(10) << "StdDev"
		<< '\n';
	file << std::string(78, '-') << '\n';

	for (const auto& [name, stats] : sortedStats)
	{
		double mean = stats.sum / stats.count;
		double variance = (stats.sumSq - stats.sum * stats.sum / stats.count) / stats.count;
		double stdDev = std::sqrt(variance);

		file << std::left << std::setw(30) << name.substr(0, 29)
			<< std::right
			<< std::setw(8) << stats.count
			<< std::setw(10) << mean
			<< std::setw(10) << stats.min
			<< std::setw(10) << stats.max
			<< std::setw(10) << stdDev
			<< '\n';
	}
	file << std::endl;
}