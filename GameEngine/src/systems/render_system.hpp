#pragma once

#include "ecs/ecs_system.hpp"

namespace Ludus::ECS
{
	class World;
}

namespace Ludus::Rendering
{
	class RenderEngine;
}

namespace Ludus::Systems
{
	class RenderSystem final : public Ludus::ECS::ISystem
	{
	public:
		static constexpr const char* ProfileName = "RenderSystem";
		static constexpr Ludus::ECS::SystemPhase Phase = Ludus::ECS::SystemPhase::RENDER;
		static constexpr int Order = 100;

		explicit RenderSystem(Ludus::Rendering::RenderEngine& renderEngine);
		void OnUpdate(Ludus::ECS::World& world) override;

	private:
		Ludus::Rendering::RenderEngine& _renderEngine;

	};
}
