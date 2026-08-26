#include "render_system.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "components/renderable.hpp"
#include "components/transform.hpp"
#include "ecs/ecs_world.hpp"
#include "rendering/render_engine.hpp"

namespace Systems
{
	RenderSystem::RenderSystem(Rendering::RenderEngine& renderEngine)
		: _renderEngine(renderEngine)
	{
	}

	void RenderSystem::OnUpdate(ECS::World& world)
	{
		world.ForEach<Components::Transform, Components::Renderable>
			([this](ECS::Entity, const Components::Transform& transform, const Components::Renderable& renderable)
				{
					if (!renderable.visible)
						return;

					Rendering::MeshInstanceDesc mesh{};
					mesh.mesh = renderable.mesh;
					mesh.materialOverride = renderable.materialOverride;

					mesh.transform = glm::translate(glm::mat4(1.f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.f), transform.scale);
					_renderEngine.Submit(mesh);
				}
			);
	}
}
