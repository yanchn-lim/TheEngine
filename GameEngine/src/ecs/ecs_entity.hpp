#pragma once

#include <cstdint>

namespace Ludus::ECS
{
	struct Entity
	{
		uint32_t id = 0;
		uint32_t generation = 0;

		bool IsValid() const noexcept
		{
			return id != 0 && generation != 0;
		}
	};

	inline bool operator==(const Entity& lhs, const Entity& rhs) noexcept
	{
		return lhs.id == rhs.id && lhs.generation == rhs.generation;
	}

	inline constexpr Entity INVALID_ENTITY = { 0, 0 };
}