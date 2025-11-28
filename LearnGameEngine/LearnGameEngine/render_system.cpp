#include "render_system.hpp"
#include "renderer2d.hpp"
#include "math.hpp"

#include <glad/glad.h>
#include <cmath>

namespace Engine
{
	RenderSystem::RenderSystem(ECS::World& world, EventBus& eventBus)
		: m_World(&world)
	{
		Renderer2D::Init();

		eventBus.Subscribe<RenderEvent>(
			[this](const RenderEvent& renderEvent)
			{
				OnRender(renderEvent);
			});
	}

	RenderSystem::~RenderSystem()
	{
		Renderer2D::Shutdown();
	}

	void RenderSystem::OnRender(const RenderEvent& renderEvent)
	{
		(void)renderEvent;

		if (!m_World) return;

		// For now, hardcode to your window size (800x600)
		const float viewportWidth = 1600.0f;
		const float viewportHeight = 900.0f;
		const float aspect = viewportWidth / viewportHeight;

		// Define a logical world height of 2 units, centered at origin
		const float worldHeight = 1.0f;
		const float worldWidth = worldHeight * aspect;

		float4x4 viewProj = MakeOrtho(
			-worldWidth * 0.5f, worldWidth * 0.5f,   // left, right
			-worldHeight * 0.5f, worldHeight * 0.5f,   // bottom, top
			-1.0f, 1.0f                                      // zNear, zFar
		);
		Renderer2D::BeginScene(viewProj);

		m_World->ForEach<ECS::Transform, ECS::Sprite2D>(
			[]( ECS::EntityId /*entity*/,
				ECS::Transform& transform,
				ECS::Sprite2D& sprite)
			{

				Renderer2D::DrawQuad(
					transform.position,
					sprite.size,
					sprite.color,
					transform.rotationZ);
			}
		);

		Renderer2D::EndScene();
	}
}