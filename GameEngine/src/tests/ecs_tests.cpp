#include "ecs_tests.hpp"

#include <vector>

#include "debug/debug.hpp"
#include "ecs/ecs_world.hpp"
#include "movement_system.hpp"
#include "position.hpp"
#include "velocity.hpp"

namespace Tests
{
	namespace
	{
		bool EcsTestFailed(const char* message)
		{
			Debug::LogError("ECS test failed: ", message);
			return false;
		}

		struct SystemTrace
		{
			int createCount = 0;
			std::vector<int> updateOrder;
			std::vector<int> fixedUpdateOrder;
			double fixedDeltaTime = 0.0;
		};

		template<ECS::SystemPhase PhaseValue, int OrderValue, int Marker>
		class OrderedSystem final : public ECS::ISystem
		{
		public:
			static constexpr ECS::SystemPhase Phase = PhaseValue;
			static constexpr int Order = OrderValue;

			explicit OrderedSystem(SystemTrace& trace) : _trace(trace) {}

			void OnCreate(ECS::World&) override { ++_trace.createCount; }
			void OnUpdate(ECS::World&) override { _trace.updateOrder.push_back(Marker); }
			void OnFixedUpdate(ECS::World&, double fixedDeltaTime) override
			{
				_trace.fixedUpdateOrder.push_back(Marker);
				_trace.fixedDeltaTime = fixedDeltaTime;
			}

		private:
			SystemTrace& _trace;
		};

		bool RunEntityAndComponentTests()
		{
			ECS::World world;
			const ECS::Entity movingEntity = world.CreateEntity();
			const ECS::Entity positionOnlyEntity = world.CreateEntity();
			if (world.GetEntityCount() != 2 || !world.IsEntityAlive(movingEntity) ||
				!world.IsEntityAlive(positionOnlyEntity))
				return EcsTestFailed("entity creation or alive count is incorrect");

			world.AddComponent(movingEntity, Position{ 10.0f, 20.0f });
			world.AddComponent(movingEntity, Velocity{ 2.0f, 3.0f });
			world.AddComponent(positionOnlyEntity, Position{ 40.0f, 50.0f });
			if (!world.HasComponent<Position>(movingEntity) ||
				!world.HasComponent<Velocity>(movingEntity) ||
				world.HasComponent<Velocity>(positionOnlyEntity))
				return EcsTestFailed("component presence is incorrect");

			if (!world.TryGetComponent<Position>(movingEntity) ||
				world.TryGetComponent<Velocity>(positionOnlyEntity))
				return EcsTestFailed("component lookup is incorrect");

			world.AddSystem<MovementSystem>();
			world.UpdateSystems();
			const Position& movedPosition = world.GetComponent<Position>(movingEntity);
			const Position& unchangedPosition = world.GetComponent<Position>(positionOnlyEntity);
			if (movedPosition.x != 12.0f || movedPosition.y != 23.0f ||
				unchangedPosition.x != 40.0f || unchangedPosition.y != 50.0f)
				return EcsTestFailed("movement query updated the wrong component data");

			std::size_t constVisitCount = 0;
			const ECS::World& constWorld = world;
			constWorld.ForEach<Position>([&](ECS::Entity, const Position&) { ++constVisitCount; });
			if (constVisitCount != 2)
				return EcsTestFailed("const query did not visit every matching entity");

			if (!world.RemoveComponent<Velocity>(movingEntity) ||
				world.RemoveComponent<Velocity>(movingEntity))
				return EcsTestFailed("component removal result is incorrect");

			world.UpdateSystems();
			const Position& positionAfterRemoval = world.GetComponent<Position>(movingEntity);
			if (positionAfterRemoval.x != 12.0f || positionAfterRemoval.y != 23.0f)
				return EcsTestFailed("system updated an entity after its required component was removed");

			world.RemoveEntity(movingEntity);
			if (world.IsEntityAlive(movingEntity) || world.GetEntityCount() != 1)
				return EcsTestFailed("entity removal is incorrect");

			const ECS::Entity reusedEntity = world.CreateEntity();
			if (reusedEntity.id != movingEntity.id ||
				reusedEntity.generation == movingEntity.generation ||
				world.IsEntityAlive(movingEntity))
				return EcsTestFailed("entity slot reuse did not invalidate the old generation");

			return true;
		}

		bool RunSystemOrderTests()
		{
			using PreUpdateSystem = OrderedSystem<ECS::SystemPhase::PREUPDATE, 500, 1>;
			using FirstUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 100, 2>;
			using SecondUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 200, 3>;
			using TiedUpdateSystem = OrderedSystem<ECS::SystemPhase::UPDATE, 200, 4>;

			ECS::World world;
			SystemTrace trace;
			world.AddSystem<SecondUpdateSystem>(trace);
			world.AddSystem<TiedUpdateSystem>(trace);
			world.AddSystem<PreUpdateSystem>(trace);
			world.AddSystem<FirstUpdateSystem>(trace);
			if (trace.createCount != 4)
				return EcsTestFailed("OnCreate did not run once for every system");

			world.UpdateSystems();
			if (trace.updateOrder != std::vector<int>{ 1, 2, 3, 4 })
				return EcsTestFailed("systems did not run by phase, order, and insertion order");

			constexpr double fixedDeltaTime = 1.0 / 60.0;
			world.FixedUpdateSystems(fixedDeltaTime);
			if (trace.fixedUpdateOrder != std::vector<int>{ 1, 2, 3, 4 } ||
				trace.fixedDeltaTime != fixedDeltaTime)
				return EcsTestFailed("fixed systems did not receive the correct order or delta time");

			return true;
		}
	}

	bool RunEcsTests()
	{
		return RunEntityAndComponentTests() && RunSystemOrderTests();
	}
}
