#include "scene.hpp"

GameObject& Scene::CreateObject()
{
	_gameObjects.emplace_back();
	return _gameObjects.back();
}

void Scene::AddObject(GameObject& obj)
{
	_gameObjects.emplace_back(obj);
}

const std::vector<GameObject>& Scene::GetGameObjects() const
{
	return _gameObjects;
}

void Scene::Update()
{

}

void Scene::Draw()
{

}
