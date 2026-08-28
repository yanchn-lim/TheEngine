#include "ecs_world.hpp"

#include <limits>
#include <algorithm>

#include "debug/profiler.hpp"

namespace Ludus::ECS
{
	World::~World()
	{
		if (_systemsNeedSorting)
			SortSystems();

		// destroy dependants before the systems that precede them.
		for (auto system = _systems.rbegin(); system != _systems.rend(); ++system)
		{
			system->instance->OnDestroy(*this);
		}
	}

	Entity World::CreateEntity()
	{
		uint32_t entityId = 0;
		uint32_t generation = 0;

		// reuse the most recently freed slot and invalidate its old handles.
		if (!_freeSlots.empty())
		{
			entityId = _freeSlots.back();
			_freeSlots.pop_back();

			Internal::EntitySlot& slot = _entitySlots[entityId];

			if (slot.generation == std::numeric_limits<uint32_t>::max())
			{
				slot.generation = 1;
			}
			else
			{
				++slot.generation;
			}

			slot.alive = true;
			generation = slot.generation;
		}
		else
		{
			entityId = static_cast<uint32_t>(_entitySlots.size());
			generation = 1;
			_entitySlots.push_back({ generation, true });
		}

		_entityAliveCount++;
		return Entity{ entityId, generation };
	}

	uint32_t World::GetEntityCount() const
	{
		return _entityAliveCount;
	}

	void World::RemoveEntity(Entity entity)
	{
		if (!IsEntityAlive(entity))
			return;

		auto& slot = _entitySlots[entity.id];
		if (slot.alive)
		{
			// remove every component before the entity slot becomes reusable.
			for(auto& pool : _componentPools)
			{
				if(pool)
					pool->RemoveIfPresent(entity);
			}

			slot.alive = false;
			_freeSlots.push_back(entity.id);
			_entityAliveCount--;

		}
	}

	bool World::IsEntityAlive(Entity entity) const
	{
		if (!entity.IsValid())
			return false;

		if (entity.id >= _entitySlots.size())
			return false;

		const Internal::EntitySlot& slot = _entitySlots[entity.id];

		return slot.alive && slot.generation == entity.generation;
	}

	void World::Swap(World& other) noexcept
	{
		using std::swap;
		swap(_entitySlots, other._entitySlots);
		swap(_freeSlots, other._freeSlots);
		swap(_componentPools, other._componentPools);
		swap(_entityAliveCount, other._entityAliveCount);
		swap(_systems, other._systems);
		swap(_nextSystemInsertionIndex, other._nextSystemInsertionIndex);
		swap(_systemsNeedSorting, other._systemsNeedSorting);
	}

	void World::SortSystems()
	{
		// insertion order makes equal phase and order values deterministic.
		// the same order applies to variable and fixed updates.
		std::ranges::sort(
			_systems,
			{},
			[](const SystemEntry& entry)
			{
				return std::tuple{
					entry.phase,
					entry.order,
					entry.insertionIndex 
				};
			});

		_systemsNeedSorting = false;
	}

	void World::UpdateSystems()
	{
		PROFILE_SCOPE("ECS Update");

		if (_systemsNeedSorting)
			SortSystems();

		for (SystemEntry& entry : _systems)
		{
			ProfileScope systemScope(entry.profileName);
			entry.instance->OnUpdate(*this);
		}
	}

	void World::FixedUpdateSystems(double fixedDeltaTime)
	{
		PROFILE_SCOPE("ECS Fixed Update");

		if (_systemsNeedSorting)
			SortSystems();

		for (SystemEntry& entry : _systems)
		{
			ProfileScope systemScope(entry.profileName);
			entry.instance->OnFixedUpdate(*this, fixedDeltaTime);
		}
	}
}
