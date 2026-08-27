#include "memory_tracker.hpp"

#if defined(ENGINE_ENABLE_MEMORY_PROFILER)

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include <new>

namespace
{
	struct AllocationHeader
	{
		void* rawPointer;
		std::size_t size;
	};

	struct TrackerState
	{
		std::atomic<std::size_t> liveBytes{};
		std::atomic<std::size_t> peakLiveBytes{};
		std::atomic<std::size_t> liveAllocationCount{};
		std::atomic<std::size_t> frameAllocatedBytes{};
		std::atomic<std::size_t> frameFreedBytes{};
		std::atomic<std::size_t> frameAllocationCount{};
		std::atomic<std::size_t> frameFreeCount{};
		Ludus::Memory::CpuMemoryStats lastFrameStats{};
	};

	struct ResourceTrackerState
	{
		std::map<std::pair<Ludus::Memory::ResourceMemoryDomain, std::string>, Ludus::Memory::ResourceMemoryStats> resources;
		std::mutex resourceMutex;
	};

	TrackerState& GetState()
	{
		// This state is allocation-free so it is safe to initialize from global new.
		static TrackerState state;
		return state;
	}

	ResourceTrackerState& GetResourceState()
	{
		// Resource bookkeeping is first needed after OpenGL resources are created.
		static ResourceTrackerState* state = []
			{
				void* storage = std::malloc(sizeof(ResourceTrackerState));
				return new(storage) ResourceTrackerState();
			}();
		return *state;
	}

	void UpdatePeak(std::atomic<std::size_t>& peak, std::size_t value)
	{
		std::size_t currentPeak = peak.load(std::memory_order_relaxed);
		while (currentPeak < value
			&& !peak.compare_exchange_weak(currentPeak, value, std::memory_order_relaxed))
		{
		}
	}

	void RecordAllocation(std::size_t size)
	{
		TrackerState& state = GetState();
		const std::size_t liveBytes = state.liveBytes.fetch_add(size, std::memory_order_relaxed) + size;
		state.liveAllocationCount.fetch_add(1, std::memory_order_relaxed);
		state.frameAllocatedBytes.fetch_add(size, std::memory_order_relaxed);
		state.frameAllocationCount.fetch_add(1, std::memory_order_relaxed);
		UpdatePeak(state.peakLiveBytes, liveBytes);
	}

	void RecordFree(std::size_t size)
	{
		TrackerState& state = GetState();
		state.liveBytes.fetch_sub(size, std::memory_order_relaxed);
		state.liveAllocationCount.fetch_sub(1, std::memory_order_relaxed);
		state.frameFreedBytes.fetch_add(size, std::memory_order_relaxed);
		state.frameFreeCount.fetch_add(1, std::memory_order_relaxed);
	}

	void AddResource(Ludus::Memory::ResourceMemoryDomain domain, std::string_view label, std::size_t bytes)
	{
		ResourceTrackerState& state = GetResourceState();
		std::lock_guard lock(state.resourceMutex);
		auto [it, inserted] = state.resources.try_emplace(
			std::make_pair(domain, std::string(label)),
			Ludus::Memory::ResourceMemoryStats{ std::string(label), domain });
		Ludus::Memory::ResourceMemoryStats& resource = it->second;
		resource.currentBytes += bytes;
		resource.resourceCount += 1;
		resource.peakBytes = std::max(resource.peakBytes, resource.currentBytes);
	}

	void RemoveResource(Ludus::Memory::ResourceMemoryDomain domain, const std::string& label, std::size_t bytes)
	{
		ResourceTrackerState& state = GetResourceState();
		std::lock_guard lock(state.resourceMutex);
		const auto it = state.resources.find(std::make_pair(domain, label));
		if (it == state.resources.end())
			return;

		Ludus::Memory::ResourceMemoryStats& resource = it->second;
		resource.currentBytes -= bytes;
		resource.resourceCount -= 1;
		if (resource.resourceCount == 0)
			state.resources.erase(it);
	}
}

namespace Ludus::Memory
{
	void BeginFrame()
	{
		TrackerState& state = GetState();
		state.frameAllocatedBytes.store(0, std::memory_order_relaxed);
		state.frameFreedBytes.store(0, std::memory_order_relaxed);
		state.frameAllocationCount.store(0, std::memory_order_relaxed);
		state.frameFreeCount.store(0, std::memory_order_relaxed);
	}

	void EndFrame()
	{
		TrackerState& state = GetState();
		CpuMemoryStats stats;
		stats.liveBytes = state.liveBytes.load(std::memory_order_relaxed);
		stats.peakLiveBytes = state.peakLiveBytes.load(std::memory_order_relaxed);
		stats.liveAllocationCount = state.liveAllocationCount.load(std::memory_order_relaxed);
		stats.frameAllocatedBytes = state.frameAllocatedBytes.load(std::memory_order_relaxed);
		stats.frameFreedBytes = state.frameFreedBytes.load(std::memory_order_relaxed);
		stats.frameAllocationCount = state.frameAllocationCount.load(std::memory_order_relaxed);
		stats.frameFreeCount = state.frameFreeCount.load(std::memory_order_relaxed);

		state.lastFrameStats = stats;
	}

	CpuMemoryStats GetLastFrameCpuStats()
	{
		TrackerState& state = GetState();
		return state.lastFrameStats;
	}

	std::vector<ResourceMemoryStats> GetResourceMemoryStats()
	{
		ResourceTrackerState& state = GetResourceState();
		std::lock_guard lock(state.resourceMutex);
		std::vector<ResourceMemoryStats> result;
		result.reserve(state.resources.size());
		for (const auto& [key, resource] : state.resources)
			result.push_back(resource);
		return result;
	}

	void* Allocate(std::size_t size, std::size_t alignment)
	{
		if (size == 0)
			size = 1;

		alignment = std::max(alignment, alignof(AllocationHeader));
		if (size > std::numeric_limits<std::size_t>::max() - sizeof(AllocationHeader) - (alignment - 1))
			return nullptr;

		void* rawPointer = std::malloc(size + sizeof(AllocationHeader) + alignment - 1);
		if (!rawPointer)
			return nullptr;

		const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(rawPointer) + sizeof(AllocationHeader);
		const std::uintptr_t alignedAddress = (start + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
		auto* header = reinterpret_cast<AllocationHeader*>(alignedAddress - sizeof(AllocationHeader));
		header->rawPointer = rawPointer;
		header->size = size;

		RecordAllocation(size);
		return reinterpret_cast<void*>(alignedAddress);
	}

	void Deallocate(void* pointer) noexcept
	{
		if (!pointer)
			return;

		auto* header = reinterpret_cast<AllocationHeader*>(reinterpret_cast<std::uintptr_t>(pointer) - sizeof(AllocationHeader));
		RecordFree(header->size);
		std::free(header->rawPointer);
	}

	ResourceUsage::~ResourceUsage()
	{
		Reset();
	}

	ResourceUsage::ResourceUsage(ResourceUsage&& other) noexcept
		: _domain(other._domain), _label(std::move(other._label)), _bytes(other._bytes)
	{
		other._bytes = 0;
	}

	ResourceUsage& ResourceUsage::operator=(ResourceUsage&& other) noexcept
	{
		if (this == &other)
			return *this;

		Reset();
		_domain = other._domain;
		_label = std::move(other._label);
		_bytes = other._bytes;
		other._bytes = 0;
		return *this;
	}

	void ResourceUsage::Set(ResourceMemoryDomain domain, std::string_view label, std::size_t bytes)
	{
		Reset();
		if (label.empty() || bytes == 0)
			return;

		_domain = domain;
		_label = label;
		_bytes = bytes;
		AddResource(_domain, _label, _bytes);
	}

	void ResourceUsage::Reset()
	{
		if (_bytes == 0)
			return;

		RemoveResource(_domain, _label, _bytes);
		_label.clear();
		_bytes = 0;
	}
}

#else

namespace Ludus::Memory
{
	void BeginFrame() {}
	void EndFrame() {}
	CpuMemoryStats GetLastFrameCpuStats() { return {}; }
	std::vector<ResourceMemoryStats> GetResourceMemoryStats() { return {}; }
	void* Allocate(std::size_t, std::size_t) { return nullptr; }
	void Deallocate(void*) noexcept {}

	ResourceUsage::~ResourceUsage() = default;
	ResourceUsage::ResourceUsage(ResourceUsage&&) noexcept = default;
	ResourceUsage& ResourceUsage::operator=(ResourceUsage&&) noexcept = default;
	void ResourceUsage::Set(ResourceMemoryDomain, std::string_view, std::size_t) {}
	void ResourceUsage::Reset() {}
}

#endif
