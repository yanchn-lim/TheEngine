#pragma once

#include "ecs.hpp"
#include "components.hpp"
#include "events.hpp"

namespace Engine
{
	class RenderSystem
	{
	public:
		RenderSystem(ECS::World& world, EventBus& eventBus);
		~RenderSystem();

	private:
		ECS::World* m_World = nullptr;

		void OnRender(const RenderEvent& renderEvent);
	};
}