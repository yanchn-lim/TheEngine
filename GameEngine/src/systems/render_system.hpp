#pragma once

#include "ecs/ecs_system.hpp"

namespace ECS
{
	class World;
}

namespace Rendering
{
	class RenderEngine;
}

namespace Systems
{
	class RenderSystem final : public ECS::ISystem
	{
	public:
		static constexpr ECS::SystemPhase Phase = ECS::SystemPhase::RENDER;
		static constexpr int Order = 100;

		explicit RenderSystem(Rendering::RenderEngine& renderEngine);
		void OnUpdate(ECS::World& world) override;

	private:
		Rendering::RenderEngine& _renderEngine;

	};
}
