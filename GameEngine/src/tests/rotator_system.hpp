#pragma once

#include "ecs/ecs_system.hpp"
#include "scene/system_registry.hpp"

namespace Tests
{
	class RotatorSystem final : public Ludus::ECS::ISystem
	{
	public:
		static constexpr Ludus::ECS::SystemPhase Phase = Ludus::ECS::SystemPhase::UPDATE;
		static constexpr int Order = 100;

		void OnFixedUpdate(Ludus::ECS::World& world, double fixedDeltaTime) override;
	};
}

namespace Ludus
{
	template<>
	struct SceneSystemCodec<Tests::RotatorSystem>
	{
		static constexpr std::string_view Id = "rotator";

		static bool Validate(
			const Ludus::Serialization::LSceneValue& config,
			std::vector<SceneLoadError>& errors);

		static void Create(
			Ludus::ECS::World& world,
			const Ludus::Serialization::LSceneValue& config);
	};
}
