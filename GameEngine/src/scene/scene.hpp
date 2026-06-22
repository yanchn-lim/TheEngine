#pragma once

#include <vector>
#include "gameobject.hpp"

class Scene
{
private:
	std::vector<GameObject> _gameObjects{};


public:
	GameObject& CreateObject();
	void AddObject(GameObject& obj);
	const std::vector<GameObject>& GetGameObjects() const;
	void Update();
	void Draw();
};
