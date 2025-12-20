#include "render_system.hpp"
#include "renderer2d.hpp"
#include "Math/math.hpp"

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

		// For now, hardcode to your window size
		const float viewportWidth = 1600.0f;
		const float viewportHeight = 900.0f;
		const float aspect = viewportWidth / viewportHeight;

		//default camera
		float cameraX = 0.0f;
		float cameraY = 0.0f;
		float cameraZoom = 1.0f;
		float nearClip = -1.0f;
		float farClip = 1.0f;

		bool hasCamera = false;

		//search for a camera
		m_World->ForEach<ECS::Transform, ECS::Camera2D>(
			[&](ECS::EntityId /*entity*/,
				ECS::Transform& camTransform,
				ECS::Camera2D& camera)
			{
				if (!hasCamera || camera.primary)
				{
					cameraX = camTransform.position.x;
					cameraY = camTransform.position.y;
					cameraZoom = camera.zoom;
					nearClip = camera.nearClip;
					farClip = camera.farClip;
					hasCamera = true;
				}
			}
		);

		// Define a logical world height of 2 units, centered at origin
		const float baseWorldHeight = 1.0f;
		const float worldHeight = baseWorldHeight / cameraZoom;
		const float worldWidth = worldHeight * aspect;

		const float left = cameraX - worldWidth * 0.5f;
		const float right = cameraX + worldWidth * 0.5f;
		const float bottom = cameraY - worldHeight * 0.5f;
		const float top = cameraY + worldHeight * 0.5f;

		float4x4 viewProj = MakeOrtho(
			left, right,
			bottom, top,
			nearClip, farClip
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