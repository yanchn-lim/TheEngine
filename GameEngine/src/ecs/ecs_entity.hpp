#pragma once

#include <cstdint>

namespace Ludus::ECS
{
	// a generation makes an Entity handle invalid after its slot is reused.
	// id 0 and generation 0 are reserved for invalid handles.
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