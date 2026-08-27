#include "render_system.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "ecs/ecs_world.hpp"
#include "rendering/render_engine.hpp"

namespace Ludus::Systems
{
	RenderSystem::RenderSystem(Ludus::Rendering::RenderEngine& renderEngine)
		: _renderEngine(renderEngine)
	{
	}

	void RenderSystem::OnUpdate(Ludus::ECS::World& world)
	{
		world.ForEach<Ludus::Components::Transform, Ludus::Components::Renderable>
			([this](Ludus::ECS::Entity, const Ludus::Components::Transform& transform, const Ludus::Components::Renderable& renderable)
				{
					if (!renderable.visible)
						return;

					Ludus::Rendering::MeshInstanceDesc mesh{};
					mesh.mesh = renderable.mesh;
					mesh.materialOverride = renderable.materialOverride;

					mesh.transform = glm::translate(glm::mat4(1.f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.f), transform.scale);
					_renderEngine.Submit(mesh);
				}
			);
	}
}
