#pragma once

#include "ecs/ecs_system.hpp"

namespace ECS
{
	class World;
}

namespace Rendering
{
	class Renderer;
}

namespace Systems
{
	class RenderSystem final : public ECS::ISystem
	{
	public:
		static constexpr ECS::SystemPhase Phase = ECS::SystemPhase::RENDER;
		static constexpr int Order = 100;

		explicit RenderSystem(Rendering::Renderer& renderer);
		void OnUpdate(ECS::World& world) override;

	private:
		Rendering::Renderer& _renderer;

	};
}