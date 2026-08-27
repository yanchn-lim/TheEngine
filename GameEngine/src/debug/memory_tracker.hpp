#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Ludus::Memory
{
	struct CpuMemoryStats
	{
		std::size_t liveBytes{};
		std::size_t peakLiveBytes{};
		std::size_t liveAllocationCount{};
		std::size_t frameAllocatedBytes{};
		std::size_t frameFreedBytes{};
		std::size_t frameAllocationCount{};
		std::size_t frameFreeCount{};
	};

	enum class ResourceMemoryDomain : uint8_t
	{
		GpuEstimated
	};

	struct ResourceMemoryStats
	{
		std::string label;
		ResourceMemoryDomain domain{};
		std::size_t currentBytes{};
		std::size_t peakBytes{};
		std::size_t resourceCount{};
	};

	void BeginFrame();
	void EndFrame();
	CpuMemoryStats GetLastFrameCpuStats();
	std::vector<ResourceMemoryStats> GetResourceMemoryStats();

	// Used only by the global C++ allocation overloads in memory_overrides.cpp.
	void* Allocate(std::size_t size, std::size_t alignment);
	void Deallocate(void* pointer) noexcept;

	class ResourceUsage
	{
	public:
		ResourceUsage() = default;
		~ResourceUsage();

		ResourceUsage(const ResourceUsage&) = delete;
		ResourceUsage& operator=(const ResourceUsage&) = delete;
		ResourceUsage(ResourceUsage&& other) noexcept;
		ResourceUsage& operator=(ResourceUsage&& other) noexcept;

		void Set(ResourceMemoryDomain domain, std::string_view label, std::size_t bytes);
		void Reset();

	private:
#if defined(ENGINE_ENABLE_MEMORY_PROFILER)
		ResourceMemoryDomain _domain{};
		std::string _label;
		std::size_t _bytes{};
#endif
	};
}
