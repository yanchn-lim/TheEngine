#pragma once

#include <vector>
#include "gameobject.hpp"

namespace Graphics
{
	class Renderer;
}

class Scene
{
private:
	std::vector<GameObject> _gameObjects{};


public:
	GameObject& CreateObject();
	void AddObject(GameObject& obj);
	void Update();
	void SubmitDrawCommands(Graphics::Renderer& renderer);
	void Draw();
};
