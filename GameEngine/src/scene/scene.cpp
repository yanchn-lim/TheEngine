#include "scene.hpp"

namespace Ludus
{
	ECS::World& Scene::GetWorld() noexcept
	{
		return _world;
	}

	const ECS::World& Scene::GetWorld() const noexcept
	{
		return _world;
	}

	void Scene::FixedUpdate(double fixedDeltaTime)
	{
		_world.FixedUpdateSystems(fixedDeltaTime);
	}

	void Scene::Update()
	{
		_world.UpdateSystems();
	}


}
