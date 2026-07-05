#include "scene.hpp"

GameObject& Scene::CreateObject()
{
	// Scene owns game objects in contiguous storage for this early architecture.
	_gameObjects.emplace_back();
	return _gameObjects.back();
}

void Scene::AddObject(GameObject& obj)
{
	// Copy an existing object into the scene-owned collection.
	_gameObjects.emplace_back(obj);
}

const std::vector<GameObject>& Scene::GetGameObjects() const
{
	// Expose read-only scene contents until systems own iteration behavior.
	return _gameObjects;
}

void Scene::Update()
{
	// Reserved for future scene-level simulation/system updates.

}

void Scene::Draw()
{
	// Reserved for future scene-to-renderer submission.

}
