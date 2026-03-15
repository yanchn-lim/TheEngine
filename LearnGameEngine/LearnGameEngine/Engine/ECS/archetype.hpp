#pragma once

#include "ECS/ecs_types.hpp"
#include <bitset>

namespace Engine
{
	namespace ECS2
	{
		using TypeId = std::uint8_t;
		using Signature = std::uint64_t; //64bitmask representation
		
		//inline Signature Bit(TypeId id) { return Signature{ 1 } << id; }

		//e.g. transform = .... 0001
		//using Signature = std::bitset<64>;

		//class Archetype
		//{
		//public:


		//private:
		//	//create pages per archetype
		//	std::vector<ArchetypePage> m_Pages;
		//};

		//each page holds a fixed size of 64kb of data
		//struct ArchetypePage
		//{
		//private:
		//	uint32 count = 0;
		//	uint32 capacity = 0;
		//	size_t maxPageSize = 65536; //64kb

		//	//array of data
		//	std::byte* data = nullptr;

		//	//column offsets
		//	//e.g.	offsets[0] = ptr to first of entity column
		//	//		offsets[1] = ptr to first of transform column
		//	//to get a transform from the data
		//	// data[offset[1] + i * size]
		//	std::vector<size_t> offsets;
		//	std::vector<const ComponentInfo> types;


		//	// ===== PAGE CREATION =====
		//	void ComputeLayout64KB(Signature )
		//	{

		//	}

		//public:
		//	ArchetypePage()
		//	{
		//		//need to calculate page size here
		//	}
		//};
	}
}