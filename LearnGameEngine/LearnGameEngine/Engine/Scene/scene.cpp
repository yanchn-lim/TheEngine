#include "Scene/scene.hpp"

namespace Engine
{
	ECS::EntityId Scene::CreateEntity()
	{
		return m_World.CreateEntity();
	}

	void Scene::DestroyEntity(ECS::EntityId entity)
	{
		m_World.DestroyEntity(entity);
	}
}